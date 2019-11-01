#ifndef RESAMPLE_PCM
#define RESAMPLE_PCM

namespace am {

	class resample_pcm
	{
	public:
		resample_pcm();
		~resample_pcm();

		int init();

		int convert();
	};
}
#endif
