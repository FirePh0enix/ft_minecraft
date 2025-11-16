// This structure need to be kept in sync with CPU code.
struct BlockState
{
    uint id_raw;

    inline uint id() {
        return id_raw & 0xFFFF;
    }
};

constexpr uint NORTH_MASK = 0b1;
constexpr uint SOUTH_MASK = 0b01;
constexpr uint WEST_MASK = 0b001;
constexpr uint EAST_MASK = 0b0001;
constexpr uint TOP_MASK = 0b00001;
constexpr uint BOTTOM_MASK = 0b000001;
