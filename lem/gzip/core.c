#include <lem.h>
#include <stdlib.h>
#include <zlib.h>

struct gzip_task {
	struct lem_async a;
	size_t len;
	unsigned char *buff;
	int err;
	lua_State *T;
};


static void
gzip_compress_work(struct lem_async *a) {
	struct gzip_task *task = (struct gzip_task*)a;

	int level = 9;
	int ret;

	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	ret = deflateInit2(&strm, level, Z_DEFLATED, 16+MAX_WBITS, 9, Z_DEFAULT_STRATEGY);

  if (ret != Z_OK) {
		task->err = ret;
  	return;
	}
	strm.avail_in = task->len;
  strm.next_in = task->buff;

	size_t out_len = task->len + 1024;
	unsigned char *out = (unsigned char*)malloc(out_len);

	strm.avail_out = out_len;
	strm.next_out = out;
	ret = deflate(&strm, Z_FINISH);

	if (ret == Z_STREAM_ERROR) {
		free(out);
		task->err = ret;
  	return;
	}
	
  (void)deflateEnd(&strm);
	task->buff = out;
	task->len = out_len - strm.avail_out;
}

static void
gzip_compress_reap(struct lem_async *a) {
	struct gzip_task *task = (struct gzip_task*)a;
	lua_State *T = task->T;

	lua_settop(T, 0);
	if (task->err != 0) {
		free(task);
		lua_pushnil(T);
		lua_pushstring(T, "compress fail");
		lem_queue(T, 2);
		return ;
	}

	lua_pushlstring(task->T, (const char*)task->buff, task->len);

	free(task->buff);
	free(task);

	lem_queue(T, 1);
}

static void
gzip_decompress_work(struct lem_async *a) {
	struct gzip_task *task = (struct gzip_task*)a;
	int ret;
	
	int out_len = task->len;
	if (out_len < 1024) {
		out_len = 1024;
	}
  unsigned char *out = lem_xmalloc(out_len);

  z_stream strm;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = task->len;
  strm.next_in = task->buff;
  ret = inflateInit2(&strm, 16+MAX_WBITS);

  if (ret != Z_OK) {
		task->err = ret;
  	return;
	}

	unsigned have = 0;
	unsigned char *out_off = out;
	unsigned available_space;
	do {
		do {
			available_space = out_len - have;

			strm.avail_out = available_space;
			strm.next_out = out_off;
			ret = inflate(&strm, Z_NO_FLUSH);
			switch (ret) {
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR;     /* and fall through */
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					(void)inflateEnd(&strm);
					task->err = ret;
					free(out);
					return ;
			}

			have += available_space - strm.avail_out;

			if (strm.avail_out == 0) {
				out_len += 8192;
				out = realloc(out, out_len);
				out_off = out;
				out_off += have;
			}

		} while (strm.avail_out == 0);
	} while (ret != Z_STREAM_END);

	(void)inflateEnd(&strm);
	task->buff = out;
	task->len = have;
}

static void
gzip_decompress_reap(struct lem_async *a) {
	struct gzip_task *task = (struct gzip_task*)a;
	lua_State *T = task->T;

	lua_settop(T, 0);
	if (task->err != 0) {
		free(task);
		lua_pushnil(T);
		lua_pushstring(T, "decompress fail");
		lem_queue(T, 2);
		return ;
	}

	lua_pushlstring(task->T, (const char*) task->buff, task->len);

	free(task->buff);
	free(task);

	lem_queue(T, 1);
}

static int
gzip_compress(lua_State *T) {
	struct gzip_task *task = lem_xmalloc(sizeof *task);
	task->err = 0;
	task->buff = (unsigned char*)lua_tolstring(T, 1, &task->len);

	lem_async_do(&task->a, gzip_compress_work, gzip_compress_reap);
	task->T = T;

	return lua_yield(T, 1);
}

static int
gzip_decompress(lua_State *T) {
	struct gzip_task *task = lem_xmalloc(sizeof *task);
	task->err = 0;
	task->buff = (unsigned char*)lua_tolstring(T, 1, &task->len);
	int offset = lua_tointeger(T, 2);
	int len = lua_tointeger(T, 3);

	if (offset > 1) {
		offset -= 1;
		task->buff += offset;
		task->len -= offset;
	} else if (offset < 0) {
    task->buff += task->len + offset;
    task->len = -offset;
  }

  if (len != 0) {
    task->len = len;
  }

	lua_settop(T, 1);
	lem_async_do(&task->a, gzip_decompress_work, gzip_decompress_reap);
	task->T = T;

	return lua_yield(T, 1);
}


int luaopen_lem_gzip_core(lua_State *L) {
	lua_newtable(L);

	lua_pushcfunction(L, gzip_compress);
	lua_setfield(L, -2, "compress");

	lua_pushcfunction(L, gzip_decompress);
	lua_setfield(L, -2, "decompress");

	return 1;
}
