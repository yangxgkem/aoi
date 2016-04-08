#include "aoi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

struct laoi_cookie {
    int count;
    int max;
    int current;
};

struct laoi_space {
    uint32_t map_id;
    float map_x;
    float map_y;
    float map_z;
    struct aoi_space * space;
    struct laoi_cookie * cookie;
};

struct laoi_cb {
    uint32_t cb_num;
};

struct laoi_objs {
	float x;
	float y;
	float z;
	float mx;
	float my;
};

static void *
aoi_alloc(void * ud, void *ptr, size_t sz) {
    struct laoi_cookie * cookie = ud;
    if (ptr == NULL) {
        void *p = malloc(sz);
        ++ cookie->count;
        cookie->current += sz;
        if (cookie->max < cookie->current) {
            cookie->max = cookie->current;
        }
        return p;
    }
    -- cookie->count;
    cookie->current -= sz;
    free(ptr);
    return NULL;
}

// 获取当前系统的微秒数
int64_t
igetcurmicro() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*1000*1000 + tv.tv_usec;
}

// 获取当前系统的毫秒数
int64_t
igetcurtick() {
	return igetcurmicro()/1000;
}


// 一堆时间宏，常用来做性能日志
#define __Millis igetcurtick()
#define __Micros  igetcurmicro()
#define __Since(t) (__Micros - t)
#define __Begin int64_t t = __Micros
#define __Stop  t = __Since(t)
#define ilog(...) printf(__VA_ARGS__)

static struct laoi_space *
_aoi_create(uint32_t map_id, float map_x, float map_y, float map_z) {
    struct laoi_space * lspace = malloc(sizeof(*lspace));
    lspace->map_id = map_id;
    lspace->map_x = map_x;
    lspace->map_y = map_y;
    lspace->map_z = map_z;
    lspace->cookie = malloc(sizeof(struct laoi_cookie));
    lspace->cookie->count = 0;
    lspace->cookie->max = 0;
    lspace->cookie->current = 0;
    lspace->space = aoi_create(aoi_alloc, lspace->cookie);

    return lspace;
}

static int
_aoi_update(struct laoi_space * lspace, uint32_t id, const char * mode, float pos_x, float pos_y, float pos_z) {
    struct aoi_space * space = lspace->space;
    if (pos_x > lspace->map_x || pos_y > lspace->map_y || pos_z > lspace->map_z ||
        pos_x < 0 || pos_y < 0 || pos_z < 0) {
            printf("aoi update pos error. map_id=>%d map=>(%f,%f,%f) pos=>(%f,%f,%f)\n",
                lspace->map_id, lspace->map_x, lspace->map_y, lspace->map_z, pos_x, pos_y, pos_z);
        	return 0;
    }
    float pos[3] = {pos_x, pos_y, pos_z};
    aoi_update(space, id, mode, pos);

    return 1;
}

static void
aoi_cb_message(void *ud, uint32_t watcher, uint32_t marker, uint8_t type) {
    struct laoi_cb * clua = ud;
    clua->cb_num++;
}

static int
_aoi_message(struct laoi_space * lspace) {
    struct aoi_space * space = lspace->space;
    struct laoi_cb clua = {0};
    aoi_message(space, aoi_cb_message, &clua);

    return 1;
}

static int
_aoi_release(struct laoi_space * lspace) {
    struct aoi_space * space = lspace->space;
    aoi_release(space);
    lspace->space = NULL;
    free(lspace->cookie);
    lspace->cookie = NULL;
    free(lspace);

    return 1;
}

static void
perf_aoi() {
	int obj_num = 1000;
	float map_x = 1024;
	float map_y = 1024;
	float map_z = 0;
	int i,ii;
	int id;
	int space_num = 10;
	int move_num = 100;
	struct laoi_space * lspace[space_num];
	struct laoi_objs objs[obj_num];
	srand((int)time(NULL));

	for (i = 0; i < 10; ++i) {
		lspace[i] = _aoi_create(1001, map_x, map_y, map_z);
		for (ii = 0; ii < obj_num; ++ii) {
			objs[ii].x = (float)(rand()%(int)(map_x-20));
			objs[ii].y = (float)(rand()%(int)(map_y-20));
			objs[ii].z = 0;
			objs[ii].mx = (float)(rand()%20);
			objs[ii].my = (float)(rand()%20);
			_aoi_update(lspace[i], ii, "wm", objs[ii].x, objs[ii].y, 0);
		}
		_aoi_message(lspace[i]);
	}

	for (i = 0; i < space_num; ++i) {
		__Begin;
		ilog("开始测试实体移动检索耗时, 场景移动实体%d个\n", move_num);
		for (ii = 0; ii < move_num; ++ii) {
			id = rand()%obj_num;
			_aoi_update(lspace[i], id, "wm", objs[id].x + objs[id].mx, objs[id].y + objs[id].my, 0);
		}
		_aoi_message(lspace[i]);
		__Stop;
		ilog("总耗时 %lld 毫秒, 平均一个实体耗时 %lld 毫秒 \n\n", t, t/move_num);
	}

}

int
main(int argc, char const *argv[]) {
	perf_aoi();
	return 0;
}
