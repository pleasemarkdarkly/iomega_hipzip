#include <extras/cddb/gn_dyn_buf.h>
#include <extras/cddb/gn_memory.h>
#include GN_STRING_H

typedef struct gn_dyn_buf_t {
	gn_str_t buffer;
	gn_uint32_t buf_size;
	gn_uint32_t max_size;
	gn_uint32_t grow_by;
} gn_dyn_buf_t;

gn_dyn_buf_ref_t
gn_dyn_buf_create(gn_uint32_t initial_size, gn_uint32_t grow_by_size)
{
	gn_dyn_buf_ref_t dyn_buf = (gn_dyn_buf_ref_t)gnmem_malloc(sizeof(gn_dyn_buf_t));

	if (dyn_buf == 0) {
		return 0;
	}

	dyn_buf->buffer = (gn_str_t)gnmem_malloc(initial_size);

	if (dyn_buf->buffer == 0) {
		gnmem_free(dyn_buf);
		return 0;
	}

	dyn_buf->buf_size = 0;
	dyn_buf->max_size = initial_size;
	dyn_buf->grow_by = grow_by_size;

	return dyn_buf;
}

gn_dyn_buf_ref_t
gn_dyn_buf_clone(gn_dyn_buf_ref_t dyn_buf)
{
	gn_dyn_buf_ref_t copy = 0;
	
	if (dyn_buf == 0) {
		return 0;
	}

	copy = (gn_dyn_buf_ref_t)gnmem_malloc(sizeof(gn_dyn_buf_t));

	if (copy == 0) {
		return 0;
	}

	copy->buffer = (gn_str_t)gnmem_malloc(dyn_buf->max_size);

	if (copy->buffer == 0) {
		gnmem_free(copy);
		return 0;
	}

	gnmem_memcpy(copy->buffer, dyn_buf->buffer, dyn_buf->buf_size);

	copy->buf_size = dyn_buf->buf_size;
	copy->max_size = dyn_buf->max_size;
	copy->grow_by = dyn_buf->grow_by;

	return copy;
}

void
gn_dyn_buf_dispose(gn_dyn_buf_ref_t dyn_buf)
{
	if (dyn_buf == 0) {
		return;
	}

	if (dyn_buf->buffer != 0) {
		gnmem_free(dyn_buf->buffer);
	}

	gnmem_free(dyn_buf);
}

gn_str_t
gn_dyn_buf_dispose_and_return_buffer(gn_dyn_buf_ref_t dyn_buf)
{
	gn_str_t buffer = 0;
	
	if (dyn_buf == 0) {
		return 0;
	}

	buffer = (gn_str_t)dyn_buf->buffer;

	gnmem_free(dyn_buf);

	return buffer;
}

gn_dyn_buf_error_t
gn_dyn_buf_append_buf(gn_dyn_buf_ref_t dyn_buf, gn_cstr_t buf, gn_uint32_t buf_len)
{
	gn_uint32_t space_available;	/* space available in buffer */

	if (dyn_buf == 0 || buf == 0) {
		return GNERR_ERR_CODE(DYN_BUF_INVALID_PARAM_ERROR);
	}

	if (buf_len <= 0) {
		return GNERR_ERR_CODE(DYN_BUF_NO_ERROR);	/* no work */
	}

	/* How much space available in the buffer? */
	space_available = dyn_buf->max_size - dyn_buf->buf_size;

	/* Expand the buffer when there isn't enough room for the new data. */
	if (space_available < buf_len) {

		gn_uint32_t space_needed;		/* expand buffer by this amount */
		gn_uint32_t larger_size;		/* size of expanded buffer */
		gn_str_t larger_buffer;			/* ptr to expanded buffer */

		space_needed = buf_len - space_available;

		/* Grow by the greater of (1) the bytes needed to accommodate
		 * the new data or (2) the standard growth increment.
		 */
		larger_size = dyn_buf->max_size
		            + ((space_needed < dyn_buf->grow_by) ? dyn_buf->grow_by : space_needed);

		larger_buffer = gnmem_realloc(dyn_buf->buffer, larger_size);

		if (larger_buffer == 0) {
			return GNERR_ERR_CODE(DYN_BUF_OUT_OF_MEMORY_ERROR);
		}

		dyn_buf->buffer = larger_buffer;
		dyn_buf->max_size = larger_size;
	}

	/* Append the new data bytes to the buffer. */
	gnmem_memcpy((void*)(dyn_buf->buffer + dyn_buf->buf_size), (void*)buf, buf_len);
	dyn_buf->buf_size += buf_len;

	return GNERR_ERR_CODE(DYN_BUF_NO_ERROR);
}

gn_dyn_buf_error_t
gn_dyn_buf_append_str(gn_dyn_buf_ref_t dyn_buf, gn_cstr_t str)
{
	gn_dyn_buf_error_t	error;

	error =  (str != 0)
	     ? gn_dyn_buf_append_buf(dyn_buf, str, strlen(str))
	     : DYN_BUF_INVALID_PARAM_ERROR;

	return GNERR_ERR_CODE(error);
}

gn_dyn_buf_error_t
gn_dyn_buf_compact(gn_dyn_buf_ref_t dyn_buf)
{
	gn_str_t compacted_buffer;

	if (dyn_buf == 0) {
		return GNERR_ERR_CODE(DYN_BUF_INVALID_PARAM_ERROR);
	}

	compacted_buffer = gnmem_realloc(dyn_buf->buffer, dyn_buf->buf_size);

	if (compacted_buffer == 0) {
		return GNERR_ERR_CODE(DYN_BUF_OUT_OF_MEMORY_ERROR);
	}

	dyn_buf->buffer = compacted_buffer;
	dyn_buf->max_size = dyn_buf->buf_size;
	
	return GNERR_ERR_CODE(DYN_BUF_NO_ERROR);
}

gn_cstr_t
gn_dyn_buf_get_data(gn_dyn_buf_ref_t dyn_buf)
{
	return (dyn_buf != 0) ? dyn_buf->buffer : 0;
}

gn_uint32_t
gn_dyn_buf_get_length(gn_dyn_buf_ref_t dyn_buf)
{
	return (dyn_buf != 0) ? dyn_buf->buf_size : 0;
}
