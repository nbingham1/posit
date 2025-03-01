#include <stdio.h>
#include <stdint.h>
#include <math.h>

#define MASK(N) ((1u<<(N))-1u)
#define BIT(V, N) (((V)>>(N))&1u);
#define NEGATE(V) ((~(V))+1u)

template <uint8_t bits, uint8_t ebits>
struct posit {
	static const int total=32;

	struct encoding {
		uint8_t neg;
		int8_t k;
		uint32_t ex;
		uint32_t fr;

		encoding(posit<bits, ebits> p) {
			neg = BIT(p.value, bits-1);
			if (neg != 0) {
				p.value = NEGATE(p.value);
			}

			uint8_t regime = __builtin_clrsb(p.value << (1+(32-bits)))+1;

			uint8_t sbits = 1;
			uint8_t rbits = regime+1;
			uint8_t fbits = bits - ebits - rbits - sbits;

			k = -(int8_t)regime;
			if (BIT(p.value, bits-sbits-1) == 1) {
				k = (int8_t)regime-1;
			}
	
			ex = (p.value >> fbits) & MASK(ebits);
			fr = p.value & MASK(fbits);
		}

		operator posit() {
			uint8_t m = k < 0 ? -k : k+1;
			uint8_t rbits = m+1;
			uint8_t fbits = bits-1-rbits-ebits;

			uint8_t regime = 1;
			if (k >= 0) {
				regime = (~regime) & MASK(rbits);
			}

			return posit{(neg << (bits-1)) | (regime << (fbits+ebits)) | (ex << fbits) | fr;
		}
	};

	uint32_t value;

	posit() {
		value = 0;
	}

	posit(double d) {
		uint64_t draw = *(uint64_t*)&d;
		uint8_t dfbits = 52;
		uint8_t debits = 11;

		uint8_t neg = draw>>63;
		int16_t e = ((draw >> dfbits) & ((1u << debits)-1u)) - ((1 << (debits-1))-1);
		uint64_t frac = draw & ((1lu << dfbits)-1);

		//printf("%f e=%d f=%lX\n", d, e, frac);

		int8_t k = e >> ebits;
		e &= (1u<<ebits)-1u;
		
		//printf("k=%d e=%d f=%lX\n", k, e, frac);

		uint8_t m = k < 0 ? -k : k+1;
		uint8_t rbits = m+1;

		uint8_t fbits = bits-1-rbits-ebits;

		//printf("%u-%u=%d\n", dfbits, fbits, (int8_t)dfbits-(int8_t)fbits);

		frac = (frac>>(dfbits - fbits)) + ((frac>>(dfbits-fbits-1))&1);
		
		//printf("m=%d rbits=%d frac=%lX\n", m, rbits, frac);

		uint8_t regime = 1;
		if (k >= 0) {
			regime = ((uint8_t)-2) & ((1u << rbits)-1u);
		}

		//printf("%X\n", regime);

		value = (neg << (bits-1)) | (regime << (fbits+ebits)) | (e << fbits) | frac;

		//printf("value=%X\n", value);

		//uint8_t regime =  
		//value = (neg << (bits-1)) | (;
	}

	operator double() {
		uint32_t tmp = value;
		uint8_t neg = (value >> (bits-1)) & 1;
		if (neg != 0) {
			tmp = (~tmp)+1;
		}

		//printf("neg %u\n", neg);

		//printf("%X  %X\n", tmp, tmp<<(1+(32-bits)));

		uint8_t regime = __builtin_clrsb(tmp << (1+(32-bits)))+1;
		//printf("regime %u\n", regime);

		uint8_t sbits = 1;
		uint8_t rbits = regime+1;
		uint8_t fbits = bits - ebits - rbits - sbits;

		//printf("bits s=%u r=%u e=%u f=%u/t=%u\n", sbits, rbits, ebits, fbits, bits);
		int8_t k = -(int8_t)regime;
		if ((tmp>>(bits-sbits-1))&1 == 1) {
			k = (int8_t)regime-1;
		}
		//printf("k=%d\n", k);
		int32_t exponent = (tmp >> fbits) & ((1u<<ebits)-1u);
		uint32_t fraction = tmp & ((1u<<fbits)-1u);
		//printf("exponent=%u fraction=%u\n", exponent, fraction);

		double frac = 1.0 + (double)fraction / (double)(1<<fbits);
		int exp = exponent + (int32_t)k*(int32_t)(1<<ebits);
		//printf("frac=%f exp=%d\n", frac, exp);

		double mag = pow((double)2.0, exp)*frac;
		if (neg) {
			return -mag;
		}
		return mag;
	}
};

template <uint8_t bits, uint8_t ebits>
posit<bits, ebits> operator+(posit<bits, ebits> p0, posit<bits, ebits> p1) {
	p0.value += p1.value;
	return p0;
}


int main() {
	posit<16, 3> p0 = 3.5;
	printf("%.10e + ", (double)p0);
	posit<16, 3> p1 = 2.5;
	printf("%.10e = ", (double)p1);
	printf("%.10e\n", (double)(p0+p1));
}
