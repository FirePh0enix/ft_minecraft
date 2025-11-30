// This structure need to be kept in sync with CPU code.
struct BlockState
{
    uint id_raw;

    inline uint id() {
        return id_raw & 0xFFFF;
    }

    inline bool isAir() {
        return id() == 0;
    }
};
