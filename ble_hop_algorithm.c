#include "stdio.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

#define max(a, b) ((a) > (b)) ? (a) : (b)
#define min(a, b) ((a) < (b)) ? (a) : (b)


uint8_t perm_8(uint8_t input)
{
	uint8_t output = 0;
	uint8_t i;

	for (i = 0; i < 8; i++) {
		output |= ((0x01 & (input >> i)) << (7 -i));
	}

	return output;
}


uint16_t perm_16(uint16_t input)
{
	uint16_t output = 0;

	output = perm_8(input);
	output |= ((perm_8(input >> 8)) << 8);

	return output;
}

uint16_t mam(uint16_t a, uint16_t b)
{
	return (17 * a + b);
}


uint16_t prn_e(uint16_t cnt, uint16_t chdtf)
{
	uint16_t output = 0;
	uint8_t  i = 0;

	// XOR
	output = cnt ^ chdtf;

	for (i = 0; i < 3; i++) {
		// PERM
		output = perm_16(output);
		// MAN
		output = mam(output, chdtf);
	}
	// XOR
	output = output ^ chdtf;

	return output;
}


void prn_e_and_s(uint16_t cnt, uint16_t chdtf, uint16_t *prn_e, uint16_t *prn_s)
{
	uint16_t output = 0;
	uint8_t i = 0;

	// XOR
	output = cnt ^ chdtf;

	for (i = 0; i < 3; i++) {
		// PERM
		output = perm_16(output);
		// MAM
		output = mam(output, chdtf);
	}

	*prn_s = output;

	// XOR
	output = output ^ chdtf;
	*prn_e = output;
}


void sub_evt_prn(uint16_t las_prn, uint16_t ch_dtf, uint16_t *prn_lu, uint16_t *prn_se)
{
	uint16_t output = 0;
	
	// PERM
	output = perm_16(las_prn);

	// MAM
	output = mam(output, ch_dtf);
	*prn_lu = output;

	// XOR
	output = output ^ ch_dtf;
	*prn_se = output;
}

uint8_t sub_evt_mapping_to_ch(uint16_t prn_se, uint16_t last_ch, uint8_t d, uint8_t n)
{
	return ((last_ch + d + ((prn_se * (n - 2*d + 1))/65535)) % n);
}

void iso_hop_algorithm(uint8_t nse, uint16_t evt_cnt, uint16_t ch_dtf, uint8_t ch_map[], uint8_t out_ch_idx[])
{
	uint16_t prn_e, prn_s;
	uint16_t prn_se, prn_lu;
	uint8_t  used_ch[37] = { 0 };
	uint8_t  ch_idx = 0;
	uint8_t  ch_i = 0;
	uint8_t  i = 0;
	uint8_t  d = 0;
	uint8_t last_idx = 0;

	// Get used channels
	for (i = 0; i < 37; i++) {
		if ((ch_map[i/8] >> (i%8) & 0x01) == 0x01)
			used_ch[ch_i++] = i;
	}

	// Get D
	d = max(1, max(min(3, ch_i - 5), min(11, (ch_i - 10)/2)));

	// unmapped event channel selection
	prn_e_and_s(evt_cnt, ch_dtf, &prn_e, &prn_s);

	ch_idx = prn_e % 37;
	// event mapping to used channel index
	for (i = 0; i < ch_i; i++) {
		if (ch_idx == used_ch[i]) {
			ch_idx = i;
			break;
		}
	}

	if (i >= ch_i) {
		ch_idx = (ch_i * prn_e)/65535;
	}	

	// subevent pseudo random number generator
	sub_evt_prn(prn_s, ch_dtf, &prn_lu, &prn_se);
	last_idx = sub_evt_mapping_to_ch(prn_se, ch_idx, d, ch_i);

	i = 0;
	out_ch_idx[i++] = used_ch[last_idx];

	while(nse-- > 1) {
		sub_evt_prn(prn_lu, ch_dtf, &prn_lu, &prn_se);
		last_idx = sub_evt_mapping_to_ch(prn_se, last_idx, d, ch_i);
		out_ch_idx[i++] = used_ch[last_idx];
	}
}

uint8_t ch_mapping_idx(uint16_t prn16_e, uint8_t chmap[])
{
	uint8_t i = 0;
	uint8_t ch_i = 0;
	uint8_t used_ch[37] = { 0 };
	uint8_t ch_idx = 0;

	// Get used channels
	for (i = 0; i < 37; i++) {
		if ((chmap[i/8] >> (i%8) & 0x01) == 0x01)
			used_ch[ch_i++] = i;
	}

	ch_idx = prn16_e % 37;
	for (i = 0; i < ch_i; i++) {
		if (ch_idx == used_ch[i])
			return used_ch[i];
	}

	// Remapping index
	ch_idx = (ch_i * prn16_e)/65535;

	return (used_ch[ch_idx]);
}

uint8_t algorithm_2(uint16_t evt_cnt, uint16_t ch_idf, uint8_t ch_map[])
{
	return ch_mapping_idx(prn_e(evt_cnt, ch_idf), ch_map);
}

int main()
{
	uint8_t chma_9[5] = {0x00, 0x06, 0xE0, 0x00, 0x1E};
	uint8_t chma[5] = {0xff, 0xff, 0xff, 0xff, 0xff};
	uint8_t ch_idx[20] = { 0 };
	uint8_t evt;

	printf("37 Used channels");
	for(; evt < 6; evt++) {
		printf("evt: %d, ch_idx: %d\r\n", evt, algorithm_2(evt, 0x305F, chma));
	}

	printf("\r\n9 Used channels\r\n");
	for (; evt <= 8; evt++) {
		printf("evt: %d, ch_idx: %d\r\n", evt, algorithm_2(evt, 0x305F, chma_9));
	}

	printf("\r\nIsochronous Channels HOP:");
	printf("\r\n37 Used channels");
	for (evt = 0; evt < 6; evt++) {
		iso_hop_algorithm(3, evt, 0x305F, chma, ch_idx);
		printf("\n|evt cnt: %2d ", evt);
		printf("  sub1: %2d ", ch_idx[0]);
		printf("sub2: %2d ", ch_idx[1]);
		printf("sub3: %2d | ", ch_idx[2]);
	}

	printf("\r\n9 Used channels");
	for (; evt <= 8; evt++) {
		//printf("evt: %d, ch_idx: %d\r\n", evt, algorithm_2(evt, 0x305F, chma));
		iso_hop_algorithm(3, evt, 0x305F, chma_9, ch_idx);
		printf("\n|evt cnt: %2d ", evt);
		printf("  sub1: %2d ", ch_idx[0]);
		printf("sub2: %2d ", ch_idx[1]);
		printf("sub3: %2d | ", ch_idx[2]);
	}

	printf("\r\n");
	return 0;
}



