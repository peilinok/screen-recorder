#ifndef D3D_HELPER
#define D3D_HELPER

#include <dxgi1_2.h>

#include <list>

namespace am {
	class d3d_helper
	{
	private:
		d3d_helper() {}

	public:
		static std::list<IDXGIAdapter*> get_adapters(int *error, bool free_lib = false);
	};
}
#endif