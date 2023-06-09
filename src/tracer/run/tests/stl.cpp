#include <stuff/trilang.hpp>

#include <gtest/gtest.h>

TEST(tracer_io, stl) {
    uint8_t test_file[] = {
      'H',  'e',  'l',  'l',  'o',  ',',  ' ',  'w',  'o',  'r',   //
      'l',  'd',  '!',  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //

      0x04, 0x00, 0x00, 0x00,  // 4 triangles

      0x7c, 0xd9, 0xa0, 0x3e,  // 0.31415925
      0x7c, 0xd9, 0xa0, 0x3e,  //
      0x7c, 0xd9, 0xa0, 0x3e,  //
      0x5f, 0x36, 0xa8, 0x3f,  // 1.31415925
      0x5f, 0x36, 0xa8, 0x3f,  //
      0x5f, 0x36, 0xa8, 0x3f,  //
      0x2f, 0x1b, 0x14, 0x40,  // 2.31415925
      0x2f, 0x1b, 0x14, 0x40,  //
      0x2f, 0x1b, 0x14, 0x40,  //
      0x2f, 0x1b, 0x54, 0x40,  // 3.31415925
      0x2f, 0x1b, 0x54, 0x40,  //
      0x2f, 0x1b, 0x54, 0x40,  //
      0x00, 0x00,              // 0 attrib bytes

      0x98, 0x0d, 0x8a, 0x40,  // 4.31415925
      0x98, 0x0d, 0x8a, 0x40,  //
      0x98, 0x0d, 0x8a, 0x40,  //
      0x98, 0x0d, 0xaa, 0x40,  // 5.31415925
      0x98, 0x0d, 0xaa, 0x40,  //
      0x98, 0x0d, 0xaa, 0x40,  //
      0x98, 0x0d, 0xca, 0x40,  // 6.31415925
      0x98, 0x0d, 0xca, 0x40,  //
      0x98, 0x0d, 0xca, 0x40,  //
      0x98, 0x0d, 0xea, 0x40,  // 7.31415925
      0x98, 0x0d, 0xea, 0x40,  //
      0x98, 0x0d, 0xea, 0x40,  //
      0x01, 0x00,              // 1 attrib byte
      0xAA,

      0xcc, 0x06, 0x05, 0x41,  // 8.31415925
      0xcc, 0x06, 0x05, 0x41,  //
      0xcc, 0x06, 0x05, 0x41,  //
      0xcc, 0x06, 0x15, 0x41,  // 9.31415925
      0xcc, 0x06, 0x15, 0x41,  //
      0xcc, 0x06, 0x15, 0x41,  //
      0xcc, 0x06, 0x25, 0x41,  // 10.31415925
      0xcc, 0x06, 0x25, 0x41,  //
      0xcc, 0x06, 0x25, 0x41,  //
      0xcc, 0x06, 0x35, 0x41,  // 11.31415925
      0xcc, 0x06, 0x35, 0x41,  //
      0xcc, 0x06, 0x35, 0x41,  //
      0x02, 0x00,              // 2 attrib bytes
      0xBB, 0xCC,
    };

    stf::trilang::stl::binary_stream stream(std::begin(test_file), std::end(test_file));
    std::vector<stf::trilang::stl::triangle> vec {};

    for (;;) {
        std::optional<stf::trilang::stl::triangle> res = stream.next();
        if (!res) {
            break;
        }
        vec.emplace_back(*res);
    }

    ASSERT_EQ(vec.size(), 3);

    for (size_t i = 0; auto tri : vec) {
        float base = 0.31415925f;

        ASSERT_FLOAT_EQ(tri.normal[0], base + i++);
        ASSERT_FLOAT_EQ(tri.vertices[0][0], base + i++);
        ASSERT_FLOAT_EQ(tri.vertices[1][0], base + i++);
        ASSERT_FLOAT_EQ(tri.vertices[2][0], base + i++);
    }
}
