#include "OpenEQ/Render/Vertex.h"

using namespace std;

BufferSegment::BufferSegment()
{
    buffer = 0;
    elementSize = 0;
    offset = 0;
    count = 0;
}

size_t BufferSegment::size() const
{
    return count * elementSize;
}

size_t BufferSegment::address() const
{
    return offset * elementSize;
}
