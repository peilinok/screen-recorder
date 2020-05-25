#include "filter.h"

namespace am {
	void format_pad_arg(char *arg, int size, const FILTER_CTX &ctx)
	{
		sprintf_s(arg, size, "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%I64x",
			ctx.time_base.num,
			ctx.time_base.den,
			ctx.sample_rate,
			av_get_sample_fmt_name(ctx.sample_fmt),
			av_get_default_channel_layout(ctx.nb_channel));
	}
}