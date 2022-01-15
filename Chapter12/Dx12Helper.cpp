#include "Dx12Helper.h"
size_t AlignmentedSize(size_t size, size_t alignment)
{
	return size + alignment - size % alignment;
}