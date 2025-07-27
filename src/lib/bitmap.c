#include <xos/bitmap.h>
#include <xos/assert.h>
#include <xos/string.h>

void bitmap_make(bitmap_t *map, char *bits, u32 length, u32 offset) {
    map->bits = bits;
    map->length = length;
    map->offset = offset;
}

void bitmap_init(bitmap_t *map, char *bits, u32 length, u32 start) {
    memset(bits, 0, length);
    bitmap_make(map, bits, length, start);
}

//测试是不是1
bool bitmap_test(bitmap_t *map, u32 index) {
    assert(index >= map->offset);

    idx_t idx = index - map->offset;

    u32 bytes = idx >> 3;    // idx / 8第几个字节
    u32 bits = idx & 0b111; // idx % 8; 字节第几位

    assert(bytes < map->length);

    return (map->bits[bytes] & (1 << bits));
}

bool bitmap_set(bitmap_t *map, u32 index, bool value) {
    assert(value == 0 || value == 1);
    assert(index >= map->offset);

    idx_t idx = index - map->offset;
    u32 bytes = idx >> 3;
    u32 bits = idx & 0b111;
    if (value) {
        map->bits[bytes] |= (1 << bits);
    } else {
        map->bits[bytes] &= ~(1 << bits);
    }
}

//得到连续count个0,并置为1
int bitmap_scan(bitmap_t *map, u32 count) {
    int start = EOF;
    u32 bits_left = map->length * 8;
    u32 next_bit = 0;
    u32 counter = 0;
    
    while (bits_left -- > 0) {
        if (!bitmap_test(map, map->offset + next_bit)) {
            counter ++;
        } else {
            counter = 0;
        }

        next_bit ++;
        if (counter == count) {
            start = next_bit - count;
            break;
        }
    }

    if (start == EOF) 
        return EOF;
    
    bits_left = count;
    next_bit = start;
    while (bits_left -- > 0) {
        bitmap_set(map, map->offset + next_bit, true);
        next_bit ++;
    }
    return start + map->offset;
}
