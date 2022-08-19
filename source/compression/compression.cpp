

#include <compression.h>
#include <quick_select.h>
#include "math.h"
#include "binary_writer.h"
#include "binary_reader.h"
#include "COBSR_RLE0.h"
#include "dwt97level.h"
#include "loglevels.h"


static void find_min_max(CMPR_FLOAT* data, size_t data_size, float &min, CMPR_FLOAT &max) {
	min = CMPR_FLOAT_POS_MAX;
	max = CMPR_FLOAT_NEG_MAX;
	for (size_t i = 0; i < data_size; i++) {
		if (data[i] > max) {
			max = data[i];
		} else if (data[i] < min) {
			min = data[i];
		}
	}
}

static int32_t encode_to_uint(CMPR_FLOAT u, CMPR_FLOAT Vmin, CMPR_FLOAT T, int32_t Q){
  u = ((u - Vmin) * T);

  // Constrain
  if (u < 0) {
      u = 0;
  } else if (u > Q) {
      u = Q;
  }

  return (int32_t) floor(u);
}

#if 0
static int32_t encode_to_uint(CMPR_FLOAT u, uint8_t Nbits, CMPR_FLOAT Vmin, CMPR_FLOAT Vmax) {
	int32_t Q = (int32_t) pow(2, Nbits) - 1;
	CMPR_FLOAT T = (Q + 1) / (Vmax - Vmin);
	u = ((u - Vmin) * T);

	// Constrain
	if (u < 0) {
		u = 0;
	} else if (u > Q) {
		u = Q;
	}

	return (int32_t) floor(u);
}
#endif

static CMPR_FLOAT decode_from_uint(int32_t u, CMPR_FLOAT Vmin, CMPR_FLOAT T){
  return u * T + Vmin;
}

#if 0
static CMPR_FLOAT decode_from_uint(int32_t u, uint8_t Nbits, CMPR_FLOAT Vmin, CMPR_FLOAT Vmax) {
	int32_t Q = (int32_t) pow(2, Nbits) - 1;
	CMPR_FLOAT T = (Vmax - Vmin) / (Q + 1);

	// Constrain
	if (u < 0) {
		u = 0;
	} else if (u > Q) {
		u = Q;
	}

	return u * T + Vmin;
}
#endif

int compress_header(comp_params_t *cp, uint8_t* comp_hdr, size_t comp_hdr_size) {
	if(cp == NULL) {
		return -1;
	}
	BinaryWriter bw;                                // CompBitSet bset(comp_hdr, comp_hdr_size);
	bw_init(&bw, comp_hdr, comp_hdr_size, NULL);

	// store compression version
	writeUINT8(&bw, cp->compression_version);   // bset.add_UINT8(cp->compression_version);
	// store frame size (number of coeff per frame)
	writeUINT32(&bw, cp->frame_size);           // bset.add_UINT32(cp->frame_size);
	// store number of coefficients preserved per frame.
	writeUINT32(&bw, cp->keep_num_coeff);       // bset.add_UINT32(cp->keep_num_coeff);
	// store number of bits used to represent each coeff.
	writeUINT8(&bw, cp->q_bits);                // bset.add_UINT8(cp->q_bits);
	// store the wavelet_name length (which must be <=255)
	int wavelet_name_length = 0;
	if(cp->wavelet_name != NULL) {
		wavelet_name_length = strlen(cp->wavelet_name);
		if(wavelet_name_length>255) {
			wavelet_name_length=255;
		}
	}
	writeUINT8(&bw, wavelet_name_length);       // bset.add_UINT8(wavelet_name_length);
	// store the wavelet name, character by character
	for(int i=0; i<wavelet_name_length; i++) {
		writeUINT8(&bw, cp->wavelet_name[i]);   // bset.add_UINT8(cp->wavelet_name.charAt(i));
	}
	// store the wavelet level
	writeUINT8(&bw, cp->wavelet_level);         // bset.add_UINT8(cp->wavelet_level);
	return (int) bw_get_size(&bw);                         // return bset.toByteArray();
}

void decompress_header(uint8_t* comp_hdr, size_t comp_hdr_size, comp_params_t *cp) {
    ErrValUINT8 result8;
    ErrValUINT32 result32;

	// TODO: Check typing to ensure there is no data loss
	BinaryReader br;                                // CompBitSet bset(compressed_header);
	br_init(&br, comp_hdr, comp_hdr_size);

	// read compression version
	result8 = readUINT8(&br);       // cp->compression_version = bset.get_UINT8();
	if(result8.error_ != ERROR_NONE){
	  return;
	}
	cp->compression_version = result8.value_;

	// read frame size (number of coeff per frame)
	result32 = readUINT32(&br);              // cp->frame_size = bset.get_UINT32();
    if(result32.error_ != ERROR_NONE){
      return;
    }
    cp->frame_size = result32.value_;

	// read number of coefficients preserved per frame.
	result32 = readUINT32(&br);          // cp->keep_num_coeff = bset.get_UINT32();
	if(result32.error_ != ERROR_NONE){
	  return;
	}
	cp->keep_num_coeff = result32.value_;

	// read number of bits used to represent each coeff.
	result8 = readUINT8(&br);                      // cp->q_bits = bset.get_UINT8();
    if(result8.error_ != ERROR_NONE){
      return;
    }
	cp->q_bits = result8.value_;
	// read the wavelet_name length (which must be <=255)
	result8 = readUINT8(&br);     // long wavelet_name_length = bset.get_UINT8();
    if(result8.error_ != ERROR_NONE){
      return;
    }
	long wavelet_name_length =result8.value_;
	// read the wavelet name, character by character
	memset(cp->wavelet_name,0,sizeof(cp->wavelet_name));
	for(int i=0; i<wavelet_name_length; i++) {
		result8 = readUINT8(&br); 		// sb.append((char)bset.get_UINT8());
	    if(result8.error_ != ERROR_NONE){
	      return;
	    }
		cp->wavelet_name[i] = result8.value_;
	}

	// read the wavelet level
	result8 = readUINT8(&br);             // cp->wavelet_level = (int) bset.get_UINT8();
    if(result8.error_ != ERROR_NONE){
      return;
    }
	cp->wavelet_level = result8.value_;
}

// Compression algorithm tailored for:
// 24bit signed integer data.
int compress_frame(CMPR_FLOAT* frame_buf, const int frame_size, const int wavelet_level,
		const int q_bits, const int keep_num_coeff, uint8_t* comp_buf, int comp_buf_size) {

    size_t orig_size = 4+4+ceil(frame_size*q_bits/8.0) + 10; // add 10 bytes for spare
    uint8_t orig_buffer[orig_size];
	BinaryWriter bw;                                // CompBitSet bset = new CompBitSet();
	bw_init(&bw, orig_buffer, orig_size, NULL);

//	// Convert the integer data to double for the wavelet transform
//	CMPR_FLOAT data[frame_size];                    // TODO: Make this NOT heap allocated!
//	for (int i = 0; i < frame_size; i++) {
//		data[i] = frame_buf[i];
//	}
	CMPR_FLOAT* data = &(frame_buf[0]);

	// Wavelet forward Transform
	fwt97level(data, frame_size, wavelet_level);

	// Calculate the cA min/max
	CMPR_FLOAT Vmin;
	CMPR_FLOAT Vmax;
	find_min_max(data, frame_size, Vmin, Vmax);

	// Store cA min/max as float (not double)
	
	writeFLOAT(&bw, Vmin);                               // bset.add_FLOAT((float) Vmin);
	writeFLOAT(&bw, Vmax);                               // bset.add_FLOAT((float) Vmax);
	
	// Compute 0 value
    int32_t Q = (int32_t) pow(2, q_bits) - 1;
    CMPR_FLOAT T = (Q + 1) / (Vmax - Vmin);
//	int quant_cA_zero = (int) encode_to_uint(0, q_bits, Vmin, Vmax);
    int quant_cA_zero = (int) encode_to_uint(0, Vmin, T, Q);

	// Sort the data
	static CMPR_FLOAT cA_sorted[/*frame_size*/ 1024];               // TODO: Make this NOT heap allocated!
	for (int i = 0; i < frame_size; i++) {
		cA_sorted[i] = abs(data[i]);
	}
	CMPR_FLOAT kval = FloydWirth_kth_descending(cA_sorted, frame_size, keep_num_coeff - 1);
	
	// find the number of duplicate 'val' values
	int kval_count = 1;
	for (int i = (keep_num_coeff - 2); i >= 0; i--) {
		if (cA_sorted[i] == kval) {
			kval_count++;
		} else {
			break;
		}
	}

	// Quantize and Store the remaining coefficients
	for (int i = 0; i < frame_size; i++) {
		CMPR_FLOAT data_abs = abs(data[i]);
		bool store_coeff = false;
		// Decide if we should keep the coefficient
		if (data_abs > kval) {
			store_coeff = true;
		} else if (data_abs == kval && kval_count > 0) {
			kval_count--;
			store_coeff = true;
		} else { // x < kval
			// do nothing
		}
		
		// quantize the coefficient
		int int_x = 0;
		if (store_coeff) {
//			int_x = encode_to_uint(data[i], q_bits, Vmin, Vmax);
		    int_x = encode_to_uint(data[i], Vmin, T, Q);
            // increment any quantized value less than quant_cA_zero.
            // to use 0 bits to represent 0.
            if(int_x < quant_cA_zero) {
                int_x++;
            }
		}

//		printf("%d: %f %d\n", i, data[i], int_x);

		writeUINT(&bw, int_x, q_bits);               // bset.add_UINT(int_x, q_bits);
	}
	
	// COBSR_RLE0 encode the coefficients
	cobsr_encode_result encode_result = cobsr_rle0_encode(comp_buf, comp_buf_size, orig_buffer, orig_size);
    int encoded_size = encode_result.out_len;

	return encoded_size;
}


void decompress_frame(uint8_t* comp_buf, const int comp_size, const int wavelet_level, const int q_bits, uint8_t* frame_buf, const int frame_size) {

	// COBSR_RLE0 decode the coefficients
    size_t decoded_size = (size_t) 4+4+ceil(frame_size*q_bits/8.0);
	uint8_t decoded_buffer[decoded_size];
	/*cobsr_decode_result decode_result =*/ cobsr_rle0_decode(decoded_buffer, decoded_size, comp_buf, comp_size);
	// TODO: handle decoding error?!?
	
	// Create a BitSet
	// CompBitSet bset = new CompBitSet(decoded_buffer);
    BinaryReader br;                                   // CompBitSet bset(compressed_header);
    br_init(&br, comp_buf, comp_size);
    ErrValINT result_int;
    ErrValFLOAT result_float;

	// Get the frame min and max values
    result_float = readFLOAT(&br);                     // float Vmin = bset.get_FLOAT();
    if(result_float.error_ != ERROR_NONE){
      return;
    }
	CMPR_FLOAT Vmin = (CMPR_FLOAT) result_float.value_;
	result_float = readFLOAT(&br);                     // float Vmax = bset.get_FLOAT();
    if(result_float.error_ != ERROR_NONE){
      return;
    }
    CMPR_FLOAT Vmax = (CMPR_FLOAT) result_float.value_;

    // Compute 0 value
    int32_t Q = (int32_t) pow(2, q_bits) - 1;
    CMPR_FLOAT T = (Q + 1) / (Vmax - Vmin);
//    int quant_cA_zero = (int) encode_to_uint(0, q_bits, Vmin, Vmax);
    int quant_cA_zero = (int) encode_to_uint(0, Vmin, T, Q);
	
	// UN-Quantize and force zero-valued coefficients
	CMPR_FLOAT unquant_coeff[frame_size];
	for(int i=0; i< frame_size; i++) {
	    result_int = readINTX(&br, q_bits);             // int int_x = (int) bset.get_UINT(q_bits);
	    if(result_int.error_ != ERROR_NONE){
	      return;
	    }
	    int32_t int_x = (int32_t)   result_int.value_;
        // decrement any quantized value less than quant_cA_zero.
        // unless the bit value is 0, which we set to quant_cA_zero.
        if(int_x == 0) {
            int_x = quant_cA_zero;
        }else if(int_x <= quant_cA_zero ) {
            int_x--;
        }
//	    unquant_coeff[i] = decode_from_uint(int_x, q_bits, Vmin, Vmax);
        unquant_coeff[i] = decode_from_uint(int_x, Vmin, T);
	}
	
	// Inverse wavelet transform
	iwt97level(unquant_coeff, frame_size, wavelet_level);
	
	// Convert the double data to int data for the wavelet transform
	for (int i = 0; i < frame_size; i++) {
		frame_buf[i] = (uint8_t) round(unquant_coeff[i]);
	}
}

CMPR_FLOAT prd(int* orig, int orig_size, int* comp, int comp_size) {
  CMPR_FLOAT num = 0;
  CMPR_FLOAT den = 0;
  if(orig_size != comp_size) {
    return DBL_MAX;
  }
  for(int i=0; i<orig_size; i++) {
    CMPR_FLOAT temp = orig[i] - comp[i];
	num += temp * temp;
	den += (CMPR_FLOAT)orig[i] * (CMPR_FLOAT)orig[i];
  }
  return sqrt(num/den)*100;
}

