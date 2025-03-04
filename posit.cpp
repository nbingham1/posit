#include <stdio.h>
#include <cstdint>
#include <math.h>

#define MASK(N) ((1u<<(N))-1u)
#define BIT(V, N) (((V)>>(N))&1u)
#define NEGATE(V) ((~(V))+1u)

struct unpacked_double {
    uint64_t fr : 52;
    int64_t ex : 11;
    uint64_t neg : 1;
} __attribute__((packed));

template <uint8_t bits, uint8_t ebits>
struct posit {
	static const int total=32;

	struct encoding {
		uint8_t neg;
		int8_t k;
		uint32_t ex;
		uint32_t fr;

		uint8_t rbits;
		uint8_t fbits;

		uint32_t encode() {
			uint8_t regime = 1;
			if (k >= 0) {
				regime = (~regime) & MASK(rbits);
			}

			uint32_t v = (regime << (fbits+ebits)) | (ex << fbits) | fr;
			return (neg << (bits-1)) | (neg ? (NEGATE(v) & MASK(bits-1)) : v);
		}

		encoding(posit<bits, ebits> p) {
			neg = BIT(p.value, bits-1);
			if (neg) {
				p.value = NEGATE(p.value);
			}

			uint8_t regime = __builtin_clrsb(p.value << (1+(32-bits)))+1;

			uint8_t sbits = 1;
			rbits = regime+1;
			fbits = bits - ebits - rbits - sbits;

			k = -(int8_t)regime;
			if (BIT(p.value, bits-sbits-1) == 1) {
				k = (int8_t)regime-1;
			}
	
			ex = (p.value >> fbits) & MASK(ebits);
			fr = p.value & MASK(fbits);
		}

		operator posit() {
			return posit{encode()};
		}

		encoding(double d) {
			unpacked_double raw = *reinterpret_cast<unpacked_double*>(&d);
			raw.ex -= MASK(10);

			neg = raw.neg;
			k = 0;
			ex = raw.ex;
			rbits = 2;
			uint8_t abits = ebits < bits-1-rbits ? ebits : bits-1-rbits;
			fbits = 31;
			fr = raw.fr >> (52-fbits-1);
			fr = (fr >> 1u) + (fr & 1u);
			do {
				k += ex >> abits;
				ex &= MASK(abits);

				uint8_t m = k < 0 ? -k : k+1;

				rbits = (m >= bits-1) ? (bits-1) : (m+1);
				abits = ebits < bits-1-rbits ? ebits : bits-1-rbits;

				uint8_t oldfbits = fbits;
				fbits = bits-1-rbits-abits;

				fr >>= (oldfbits-fbits-1);
				fr = (fr >> 1u) + (fr & 1u);
				// ...(2^ex)*(1+fr)
				if (fr >> fbits) {
					fr = ((fr+(1u<<fbits))>>1)-(1u<<fbits);
					ex += 1;
				}
			} while ((ex >> abits) > 0 and fbits > 0);
		}

		operator double() {
			unpacked_double raw;
			raw.neg = neg;
			raw.fr = ((uint64_t)fr) << (52-fbits);
			raw.ex = ex + (int32_t)k*(int32_t)(1<<ebits) + MASK(10);
			return *reinterpret_cast<double*>(&raw);
		}
	};

	uint32_t value;

	posit() {
	}

	posit(uint32_t value) {
		this->value = value;
	}

	posit(double d) {
		value = encoding(d).encode();
	}

	operator double() {
		return encoding(*this);
	}

	posit<bits, ebits> sigmoid() {
		if (ebits != 0) {
			printf("error! this function only works with 0 exponent bits\n");
		}
		posit<bits, ebits> result((value ^ (1u<<(bits-1u))) >> 2);
		return result;
	}
};

/*template <uint8_t bits, uint8_t ebits>
posit<bits, ebits> operator+(posit<bits, ebits> p0, posit<bits, ebits> p1) {
	posit<bits, ebits> encoding e0(p0), e1(p1);
	int8_t dk = e0.k - e1.k;
	if (e1.k > e0.k) {
		e0.k = e1.k;
	}
	return p0;
}*/

int main() {
	for (double x = -10.0; x < 10.0; x += 0.01) {
		posit<8, 0> p(x);
		double xp = p;
		double y = p.sigmoid();
		printf("%.10le\t%.10e\t%.10e\n", x, xp, y);
	}
	/*posit<16, 3> p0 = 3.5;
	printf("%.10e + ", (double)p0);
	posit<16, 3> p1 = 2.5;
	printf("%.10e = ", (double)p1);
	printf("%.10e\n", (double)(p0+p1));*/
}
