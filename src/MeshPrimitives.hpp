#pragma once

#include "Render/Driver.hpp"

inline Expected<Ref<Mesh>> create_cube_with_separate_faces(glm::vec3 size = glm::vec3(1.0), glm::vec3 offset = {})
{
    const glm::vec3 hs = size / glm::vec3(2.0);

    std::array<uint16_t, 36> indices{
        0, 1, 2, 2, 3, 0,       // front
        20, 21, 22, 22, 23, 20, // back
        4, 5, 6, 6, 7, 4,       // right
        12, 13, 14, 14, 15, 12, // left
        8, 9, 10, 10, 11, 8,    // top
        16, 17, 18, 18, 19, 16, // bottom
    };

    std::array<glm::vec3, 36> vertices{{
        {-hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z}, // front
        {hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z},
        {hs.x + offset.x, hs.y + offset.y, hs.z + offset.z},
        {-hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z},

        {hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z}, // back
        {-hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z},
        {-hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z},
        {hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z},

        {-hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z}, // left
        {-hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z},
        {-hs.x + offset.x, hs.y + offset.y, hs.z + offset.z},
        {-hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z},

        {hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z}, // right
        {hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z},
        {hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z},
        {hs.x + offset.x, hs.y + offset.y, hs.z + offset.z},

        {-hs.x + offset.x, hs.y + offset.y, hs.z + offset.z}, // top
        {hs.x + offset.x, hs.y + offset.y, hs.z + offset.z},
        {hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z},
        {-hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z},

        {-hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z}, // bottom
        {hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z},
        {hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z},
        {-hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z},
    }};

    std::array<glm::vec2, 36> uvs{{
        {0.0, 0.0},
        {1.0, 0.0},
        {1.0, 1.0},
        {0.0, 1.0},

        {0.0, 0.0},
        {1.0, 0.0},
        {1.0, 1.0},
        {0.0, 1.0},

        {0.0, 0.0},
        {1.0, 0.0},
        {1.0, 1.0},
        {0.0, 1.0},

        {0.0, 0.0},
        {1.0, 0.0},
        {1.0, 1.0},
        {0.0, 1.0},

        {0.0, 0.0},
        {1.0, 0.0},
        {1.0, 1.0},
        {0.0, 1.0},

        {0.0, 0.0},
        {1.0, 0.0},
        {1.0, 1.0},
        {0.0, 1.0},
    }};

    std::array<glm::vec3, 36> normals{{
        {0.0, 0.0, 1.0}, // front
        {0.0, 0.0, 1.0},
        {0.0, 0.0, 1.0},
        {0.0, 0.0, 1.0},

        {0.0, 0.0, -1.0}, // back
        {0.0, 0.0, -1.0},
        {0.0, 0.0, -1.0},
        {0.0, 0.0, -1.0},

        {1.0, 0.0, 0.0}, // left
        {1.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},

        {-1.0, 0.0, 0.0}, // right
        {-1.0, 0.0, 0.0},
        {-1.0, 0.0, 0.0},
        {-1.0, 0.0, 0.0},

        {0.0, 1.0, 0.0}, // top
        {0.0, 1.0, 0.0},
        {0.0, 1.0, 0.0},
        {0.0, 1.0, 0.0},

        {0.0, -1.0, 0.0}, // bottom
        {0.0, -1.0, 0.0},
        {0.0, -1.0, 0.0},
        {0.0, -1.0, 0.0},
    }};

    Span<uint16_t> indices_span = indices;

    return RenderingDriver::get()->create_mesh(IndexType::Uint16, indices_span.as_bytes(), vertices, uvs, normals);
}
