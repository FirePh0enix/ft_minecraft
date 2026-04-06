# Raytracing implementation

Each chunk slice can have its own Sparse-Voxel-Tree with two levels using the following structure.
```cpp
struct SvtNode {
    uint32_t leaf       : 1;
    uint32_t child_ptr  : 31;
    uint64_t child_mask : 64;
}
```

During my first implementation, I forgot about negative coordinates which are allowed in the game but dont mix well
with the raytracing algorithm.
But raytracing could be down in the chunk coordinate space instead which don't allow negatives values.
