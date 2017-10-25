#include <stdlib.h>
#include <stdio.h>
#include <map>
#include <string>
#include <png.h>
#include <time.h>
#include <cstring>

#define UTF8_FULL_BLOCK   "\xE2\x96\x88"
#define UTF8_DARK_SHADE   "\xE2\x96\x93"
#define UTF8_MEDIUM_SHADE "\xE2\x96\x92"
#define UTF8_LIGHT_SHADE  "\xE2\x96\x91"
#define UTF8_NO_BLOCK     " "

struct ansi_color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
};

const struct ansi_color color_table[] =
{
    {  0,   0,   0}, // black
    {128,   0,   0}, // red
    {  0, 128,   0}, // green
    {128, 128,   0}, // yellow
    {  0,   0, 128}, // blue
    {128,   0, 128}, // magenta
    {  0, 128, 128}, // cyan
    {192, 192, 192}  // white
};

const struct ansi_color bright_color_table[] =
{
    {128, 128, 128}, // black
    {255,   0,   0}, // red
    {  0, 255,   0}, // green
    {255, 255,   0}, // yellow
    {  0,   0, 255}, // blue
    {255,   0, 255}, // magenta
    {  0, 255, 255}, // cyan
    {255, 255, 255}  // white
};

int width, height;
png_byte color_type;
png_byte bit_depth;
png_bytep *row_pointers;

void read_png_file(char *filename) {
    FILE *fp = fopen(filename, "rb");

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png) abort();

    png_infop info = png_create_info_struct(png);
    if(!info) abort();

    if(setjmp(png_jmpbuf(png))) abort();

    png_init_io(png, fp);

    png_read_info(png, info);

    width      = png_get_image_width(png, info);
    height     = png_get_image_height(png, info);
    color_type = png_get_color_type(png, info);
    bit_depth  = png_get_bit_depth(png, info);

    // Read any color_type into 8bit depth, RGBA format.
    // See http://www.libpng.org/pub/png/libpng-manual.txt

    if(bit_depth == 16)
    png_set_strip_16(png);

    if(color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png);

    // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
    if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png);

    if(png_get_valid(png, info, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png);

    // These color_type don't have an alpha channel then fill it with 0xff.
    if(color_type == PNG_COLOR_TYPE_RGB ||
     color_type == PNG_COLOR_TYPE_GRAY ||
     color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

    if(color_type == PNG_COLOR_TYPE_GRAY ||
     color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png);

    png_read_update_info(png, info);

    row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
    for(int y = 0; y < height; y++) {
    row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
    }

    png_read_image(png, row_pointers);

    fclose(fp);
}

void ansi2rgb(bool bright, int fg,  int bg, int shade, unsigned char *r, unsigned char *g, unsigned char *b) {
    double opacity;
    bg %= 10;
    fg %= 10;
    switch (shade) {
        case 0:  opacity = 1.00; break;
        case 1:  opacity = 0.75; break;
        case 2:  opacity = 0.50; break;
        case 3:  opacity = 0.25; break;
        default: opacity = 0.00; break;
    }
    unsigned char r_fg, g_fg, b_fg;
    unsigned char r_bg, g_bg, b_bg;

    if (bright) {
        r_fg = bright_color_table[fg].r;
        g_fg = bright_color_table[fg].g;
        b_fg = bright_color_table[fg].b;
    }
    else {
        r_fg = color_table[fg].r;
        g_fg = color_table[fg].g;
        b_fg = color_table[fg].b;
    }

    r_bg = color_table[bg].r;
    g_bg = color_table[bg].g;
    b_bg = color_table[bg].b;

    *r = (unsigned char)(opacity * r_fg + (1.0 - opacity) * r_bg);
    *g = (unsigned char)(opacity * g_fg + (1.0 - opacity) * g_bg);
    *b = (unsigned char)(opacity * b_fg + (1.0 - opacity) * b_bg);
}

unsigned char dist(unsigned char x1, unsigned char x2) {
     if (x1 > x2) return x1 - x2;
     return x2 - x1;
}

size_t color_index(unsigned char R, unsigned char G, unsigned char B, std::map<size_t, std::string> *palette) {
    size_t best_index    = 0;
    size_t best_distance = ~0;
    size_t index;
    unsigned char r,g,b;
    for (std::map<size_t, std::string>::iterator it=palette->begin(); it!=palette->end(); ++it) {
        index = it->first;
        r = index % 256;
        g = (index / 256) % 256;
        b = (index / 256) / 256;
        size_t rd = dist(R, r);
        size_t gd = dist(G, g);
        size_t bd = dist(B, b);
        size_t d  = rd*rd + gd*gd + bd*bd;
        if ( d < best_distance ) {
            best_distance = d;
            best_index = index;
        }
    }

    return best_index;
}

int main(int argc, char** argv) {
    srand (time(NULL));

    const char * shades[5];
    shades[0] = UTF8_FULL_BLOCK;
    shades[1] = UTF8_DARK_SHADE;
    shades[2] = UTF8_MEDIUM_SHADE;
    shades[3] = UTF8_LIGHT_SHADE;
    shades[4] = UTF8_NO_BLOCK;

    if(argc != 2) {
        printf("Usage: %s <png file>\n", argv[0]);
        abort();
    }

    printf("Generating color table.\n");
    std::map<size_t, std::string> palette;
    char buf[256];

    for (int bg=40; bg<=47; ++bg) {
        for (int fg=30; fg<=37; ++fg) {
            for (int shade=0; shade<5; ++shade) {
                unsigned char r,g,b;
                ansi2rgb(true, fg, bg, shade, &r, &g, &b);
                size_t index = r + 256*g + 65536*b;
                sprintf(buf, "\x1B[1;%d;%dm%s",fg, bg, shades[shade]);
                palette[index] = buf;
                //palette.insert(r + 256*g + 65536*b);
                //printf("\x1B[1;%d;%dm%s(%3d, %3d, %3d)",fg, bg, shades[shade], r, g, b);
            }
        }
        //printf("\x1B[0m\n");
    }

    for (int bg=40; bg<=47; ++bg) {
        for (int fg=30; fg<=37; ++fg) {
            for (int shade=0; shade<5; ++shade) {
                unsigned char r,g,b;
                ansi2rgb(false, fg, bg, shade, &r, &g, &b);

                size_t index = r + 256*g + 65536*b;
                sprintf(buf, "\x1B[22;%d;%dm%s",fg, bg, shades[shade]);
                palette[index] = buf;

                //palette.insert(r + 256*g + 65536*b);
                //printf("\x1B[%d;%dm%s(%3d, %3d, %3d)",fg, bg, shades[shade], r, g, b);
            }
        }
        //printf("\x1B[0m\n");
    }
    printf("Palette contains %lu colors.\n", palette.size());

    printf("Reading %s.\n", argv[1]);
    read_png_file(argv[1]);

    std::string last_ansi;
    for(int y = 0; y < height; y++) {
        png_bytep row = row_pointers[y];
        if (!last_ansi.empty()) {
            for (size_t i=0; i<last_ansi.size(); ++i) {
                printf("%c", last_ansi.at(i));
                if (last_ansi[i] == 'm') break;
            }
        }
        for(int x = 0; x < width; x++) {
            png_bytep px = &(row[x * 4]);
            // Do something awesome for each pixel here...
            if (px[3] < 128) {
                strcpy(buf, "\x1B[0m ");

                if (last_ansi.compare(buf) != 0) {
                    last_ansi = buf;
                    printf("%s", buf);
                }
                else printf(" ");
            }
            else {
                size_t index = color_index(px[0], px[1], px[2], &palette);
                sprintf(buf, "%s", palette[index].c_str());

                if (last_ansi.compare(buf) != 0) {
                    last_ansi = buf;
                    printf("%s", buf);
                }
                else {
                    std::string block;
                    bool data = false;
                    for (size_t i=0; last_ansi.c_str()[i] != '\0'; ++i) {
                        if (last_ansi.c_str()[i] == 'm') {
                            data = true;
                            continue;
                        }
                        if (data) block.append(1, last_ansi.c_str()[i]);
                    }
                    printf("%s", block.c_str());
                }
            }
            //printf("%4d, %4d = RGBA(%3d, %3d, %3d, %3d)\n", x, y, px[0], px[1], px[2], px[3]);
        }
        printf("\x1B[0m\n");
    }

    return 0;
}



