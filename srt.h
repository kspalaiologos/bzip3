
#ifndef _MTF_H
#define _MTF_H

static const int MAX_HDR_SIZE = 4 * 256;

struct srt_state {
    uint32_t freqs[256];
    uint8_t symbols[256];
    uint32_t r2s[256];
    uint32_t s2r[256];
    uint32_t buckets[256];
    uint32_t bucket_ends[256];
};

static int preprocess(const uint32_t * freqs, uint8_t * symbols) {
    int nb_symbols = 0;
    for(int i = 0; i < 256; i++)
        if(freqs[i] > 0)
            symbols[nb_symbols++] = i;
    uint32_t h = 4;
    while(h < nb_symbols)
        h = h * 3 + 1;
    while(1) {
        h /= 3;
        for(uint32_t i = h; i < nb_symbols; i++) {
            const int t = symbols[i] & 0xFF;
            int32_t b = i - h;
            while((b >= 0) && freqs[symbols[b]] < freqs[t]
            || (freqs[t] == freqs[symbols[b]]) && t < symbols[b])
                { symbols[b + h] = symbols[b]; b -= h; }
            symbols[b + h] = t;
        }
        if(h == 1)
            break;
    }
    return nb_symbols;
}

static int encode_header(uint32_t * freqs, uint8_t * dst) {
    uint32_t idx = 0;
    for(int i = 0; i < 256; i++) {
        uint32_t f = freqs[i];
        while(f >= 128) {
            dst[idx++] = (uint8_t) (f | 0x80);
            f >>= 7;
        }
        dst[idx++] = (uint8_t) f;
    }
    return idx;
}

static int decode_header(uint8_t * src, uint32_t * freqs) {
    uint32_t idx = 0;
    for(int i = 0; i < 256; i++) {
        int val = src[idx++] & 0xFF;
        int res = val & 0x7F;
        int shift = 7;
        while(val >= 128) {
            val = src[idx++] & 0xFF;
            res |= (val & 0x7F) << shift;
            if(shift > 21)
                break;
            shift += 7;
        }
        freqs[i] = res;
    }
    return idx;
}

uint32_t srt_encode(struct srt_state * mtf, uint8_t *src, uint8_t *dst, uint32_t count) {
    // Find first symbols and build a histogram.
    for(int i = 0; i < 256; i++)
        mtf->freqs[i] = 0;
    for(uint32_t i = 0, b = 0; i < count;) {
        if(mtf->freqs[src[i]] == 0) {
            mtf->r2s[b] = src[i];
            mtf->s2r[src[i]] = b;
            b++;
        }
        uint32_t j = i + 1;
        while(j < count && src[j] == src[i])
            j++;
        mtf->freqs[src[i]] += j - i;
        i = j;
    }

    int n_symbols = preprocess(mtf->freqs, mtf->symbols);
    for(uint32_t i = 0, bucket_pos = 0; i < n_symbols; i++) {
        mtf->buckets[mtf->symbols[i]] = bucket_pos;
        bucket_pos += mtf->freqs[mtf->symbols[i]];
    }

    const uint32_t header_size = encode_header(mtf->freqs, dst);
    const int dst_idx = header_size;
    for(uint32_t i = 0; i < count; ) {
        const int c = src[i] & 0xFF;
        int r = mtf->s2r[c] & 0xFF;
        uint32_t p = mtf->buckets[c];
        dst[dst_idx + p++] = r;
        if(r != 0) {
            do {
                mtf->r2s[r] = mtf->r2s[r - 1];
                mtf->s2r[mtf->r2s[r]] = r;
                r--;
            } while(r != 0);
            mtf->r2s[0] = c;
            mtf->s2r[c] = 0;
        }
        i++;
        while(i < count && src[i] == c) {
            dst[dst_idx + p++] = 0;
            i++;
        }
        mtf->buckets[c] = p;
    }
    return count + header_size;
}

uint32_t srt_decode(struct srt_state * mtf, uint8_t *src, uint8_t *dst, uint32_t count) {
    const uint32_t header_size = decode_header(src, mtf->freqs);
    const uint32_t src_idx = header_size;
    int nb_symbols = preprocess(mtf->freqs, mtf->symbols);
    for(uint32_t i = 0, bucket_pos = 0; i < nb_symbols; i++) {
        const int c = mtf->symbols[i] & 0xFF;
        mtf->r2s[src[src_idx + bucket_pos] & 0xFF] = c;
        mtf->buckets[c] = bucket_pos + 1;
        bucket_pos += mtf->freqs[c];
        mtf->bucket_ends[c] = bucket_pos;
    }
    uint32_t c = mtf->r2s[0];
    for(uint32_t i = 0; i < count; i++) {
        dst[i] = c;
        if(mtf->buckets[c] < mtf->bucket_ends[c]) {
            const int r = src[src_idx + mtf->buckets[c]] & 0xFF;
            mtf->buckets[c]++;
            if(r == 0)
                continue;
            for(int s = 0; s < r; s++)
                mtf->r2s[s] = mtf->r2s[s + 1];
            mtf->r2s[r] = c;
            c = mtf->r2s[0];
        } else {
            if(nb_symbols == 1)
                continue;
            nb_symbols--;
            for(int s = 0; s < nb_symbols; s++)
                mtf->r2s[s] = mtf->r2s[s + 1];
            c = mtf->r2s[0];
        }
    }
    return count - header_size;
}

#endif
