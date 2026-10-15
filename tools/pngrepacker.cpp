// SPDX-License-Identifier: Apache-2.0 + LLVM exception
// Copyright 2026 Jussi Pakkanen

#include <pystd2026/thread_executor.hpp>
#include <png.h>
#include <zlib.h>

void write_png(pystd2026::Bytes &pixels, int w, int h, const char *outfile) {
    FILE *f = fopen(outfile, "wb");
    if(!f) {
        fprintf(stderr, "Could not open output file %s.\n", outfile);
        return;
    }

    png_struct *p = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

    png_info *i = png_create_info_struct(p);

    png_init_io(p, f);

    png_set_compression_level(p, Z_BEST_COMPRESSION);
    png_set_IHDR(p,
                 i,
                 w,
                 h,
                 8,
                 PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    pystd2026::Vector<unsigned char *> rowptrs;
    rowptrs.reserve(h);
    for(int j = 0; j < h; ++j) {
        rowptrs.push_back((unsigned char *)pixels.data() + 3 * w * j);
    }
    png_set_rows(p, i, rowptrs.data());
    png_write_png(p, i, PNG_TRANSFORM_IDENTITY, nullptr);

    fclose(f);
    png_destroy_info_struct(p, &i);
    png_destroy_write_struct(&p, &i);
}

void repack_one(const char *infile, const char *outfile) {
    FILE *f = fopen(infile, "rb");
    if(!f) {
        fprintf(stderr, "Could not open file: %s\n", infile);
        return;
    }
    png_struct *p = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_info *i = png_create_info_struct(p);

    png_init_io(p, f);
    png_read_png(p, i, PNG_TRANSFORM_PACKING, nullptr);
    // In case of errors just let these leak. This is only a test app after all.

    const auto w = png_get_image_width(p, i);
    const auto h = png_get_image_height(p, i);

    const auto image_format = png_get_color_type(p, i);
    if(image_format != PNG_COLOR_TYPE_RGB) {
        fprintf(stderr, "Unsupported png file format %d.\n", (int)image_format);
        return;
    }

    pystd2026::Bytes pixels(w * h * 3);
    unsigned char **rows = png_get_rows(p, i);
    for(uint32_t row_number = 0; row_number < h; ++row_number) {
        auto *current_row = rows[row_number];
        for(size_t i = 0; i < w * 3; i += 3) {
            pixels.push_back(current_row[i]);
            pixels.push_back(current_row[i + 1]);
            pixels.push_back(current_row[i + 2]);
        }
    }

    write_png(pixels, w, h, outfile);

    fclose(f);
    png_destroy_info_struct(p, &i);
    png_destroy_read_struct(&p, nullptr, nullptr);
}

int main(int argc, char **argv) {
    repack_one("tmp/capture00000000.png", "tmp2/out0.png");
    return 0;
}
