/////
///   get_videoinfo.c
///
///   Written by Simon Chun (simon.chun@samsung.com)
///   2008/06/16
///

#include <stdio.h>
#include <string.h>


#define NAL_UNIT_TYPE_TYPE(n)    ((0x001F) & (unsigned int) (n))


static int find_one(unsigned int bits8)
{
    if (bits8 >= 8)
        return 4;
    else if (bits8 >= 4)
        return 3;
    else if (bits8 >= 2)
        return 2;
    else if (bits8 == 1)
        return 1;

    return 0;
}

static int num_bits(unsigned int bits)
{
    if (bits & 0xFFFF0000) {
        if (bits & 0xF0000000) {
            return (find_one(bits >> 28) + 28);
        }
        else if (bits & 0x0F000000) {
            return (find_one(bits >> 24) + 24);
        }
        else if (bits & 0x00F00000) {
            return (find_one(bits >> 20) + 20);
        }
        else {
            return (find_one(bits) + 16);
        }
    }
    else if (bits & 0x0000FFFF) {
        if (bits & 0x0000F000) {
            return (find_one(bits >> 12) + 12);
        }
        else if (bits & 0x00000F00) {
            return (find_one(bits >> 8) + 8);
        }
        else if (bits & 0x000000F0) {
            return (find_one(bits >> 4) + 4);
        }
        else {
            return find_one(bits);
        }
    }

    return 0;
}

static unsigned int read_bits(unsigned char bytes[], int num_read, int *bit_offset)
{
    unsigned int   bits;
    unsigned int   utmp;

    int            i;
    int            bit_shift;

    int   byte_offset, bit_offset_in_byte;
    int   num_bytes_copy;


    if (num_read > 24)    // Max 24 bits까지만 지원된다.
        return 0xFFFFFFFF;
    if (num_read == 0)
        return 0;


    // byte_offset과
    // 그 byte 내에서 bit_offset을 구한다.
    byte_offset = (*bit_offset) >> 3;    // byte_offset = (*bit_offset) / 8
    bit_offset_in_byte = (*bit_offset) - (byte_offset << 3);


    num_bytes_copy = ((*bit_offset + num_read) >> 3) - (*bit_offset >> 3) + 1;
    bits = 0;
    for (i=0; i<num_bytes_copy; i++) {
        utmp = bytes[byte_offset + i];
        bits = (bits << 8) | (utmp);
    }

    bit_shift = (num_bytes_copy << 3) - (bit_offset_in_byte + num_read);
    bits >>= bit_shift;
    bits &= (0xFFFFFFFF >> (32 - num_read));

    *bit_offset += num_read;

    return bits;
}

// unsigned integer Exp-Golomb-codec element with the left bit first.
// The parsing process for this descriptor is specified in subclause 9.1 (ITU-T Rec. H.264).
static unsigned int ue_v(unsigned char bytes[], int *bit_offset)
{
    unsigned int   b;
    int            leadingZeroBits = -1;
    unsigned int   codeNum;


    for (b=0; !b; leadingZeroBits++) {
        b = read_bits(bytes, 1, bit_offset);
    }

    // codeNum = 2^(leadingZeroBits) - 1 + read_bits(leadingZeroBits)
    codeNum = (1 << leadingZeroBits) - 1 + read_bits(bytes, leadingZeroBits, bit_offset);


    return codeNum;
}

// signed integer Exp-Golomb-codec element with the left bit first.
// The parsing process for this descriptor is specified in subclause 9.1 (ITU-T Rec. H.264).
static signed int se_v(unsigned char bytes[], int *bit_offset)
{
    signed int   b;
    int          leadingZeroBits = -1;
    signed int   codeNum;


    for (b=0; !b; leadingZeroBits++) {
        b = read_bits(bytes, 1, bit_offset);
    }

    // codeNum = 2^(leadingZeroBits) - 1 + read_bits(leadingZeroBits)
    codeNum = (1 << leadingZeroBits) - 1 + read_bits(bytes, leadingZeroBits, bit_offset);


    return codeNum;
}

static unsigned int u_n(unsigned char bytes[], int n_bits, int *bit_offset)
{
    return read_bits(bytes, n_bits, bit_offset);
}


int get_h264_info(unsigned char *sps, int sps_leng, int *profile_idc, int *width, int *height)
{
    int                loop_exit = 0;
    unsigned int       delimiter;
    unsigned char     *pdat, *p_vos_end;


    int                j;
    int                bit_offset;
    unsigned char      utmp, utmp2;
    int                itmp;

    unsigned char      PicHeightInMapUnits;
    unsigned char      FrameHeightInMbs;
    unsigned char      PicHeightInMbs;

    unsigned int       log2_max_frame_num_minus4 = 0;
    unsigned int       frame_mbs_only_flag = 0;


    pdat       = sps;
    p_vos_end  = sps + sps_leng;
    do {

        delimiter = pdat[0];
        delimiter = (delimiter << 8) | pdat[1];
        delimiter = (delimiter << 8) | pdat[2];
        delimiter = (delimiter << 8) | pdat[3];
        pdat += 4;


        while (pdat != p_vos_end) {
            if (delimiter == 0x00000001)
                break;

            delimiter = (delimiter << 8) | *pdat;
            pdat++;
        }

        switch (NAL_UNIT_TYPE_TYPE(*pdat)) {
        case 7:        // SPS (Sequence Parameter Set)
            pdat++;
            bit_offset = 0;


            *profile_idc = u_n(pdat, 8, &bit_offset);        // profile_idc

            utmp = u_n(pdat, 1, &bit_offset);        // constraint_set0_flag
            utmp = u_n(pdat, 1, &bit_offset);        // constraint_set1_flag
            utmp = u_n(pdat, 1, &bit_offset);        // constraint_set2_flag
            utmp = u_n(pdat, 1, &bit_offset);        // constraint_set3_flag
            utmp = u_n(pdat, 4, &bit_offset);        // reserved_zero_4bits
            utmp = u_n(pdat, 8, &bit_offset);        // level_idc


            utmp = ue_v(pdat, &bit_offset);            // seq_parameter_set_id
            if ((*profile_idc == 100) || (*profile_idc == 110) ||
                (*profile_idc == 122) || (*profile_idc == 144)) {

                utmp = ue_v(pdat, &bit_offset);        // chroma_format_idc
                if( utmp == 3 )
                    utmp = u_n(pdat, 1, &bit_offset);        // residual_colour_transform_flag
                utmp = ue_v(pdat, &bit_offset);        // bit_depth_luma_minus8
                utmp = ue_v(pdat, &bit_offset);        // bit_depth_chroma_minus8
                utmp = u_n(pdat, 1, &bit_offset);        // qpprime_y_zero_transform_bypass_flag
                utmp = u_n(pdat, 1, &bit_offset);        // seq_scaling_matrix_present_flag

                if (utmp) {
                    return -1;        // In this case, the parsing routine does not support.
                }
                    
            }


            log2_max_frame_num_minus4
                = ue_v(pdat, &bit_offset);            // log2_max_frame_num_minus4


            utmp = ue_v(pdat, &bit_offset);            // pic_order_cnt_type
            if (utmp == 0)
                utmp = ue_v(pdat, &bit_offset);        // log2_max_pic_order_cnt_lsb_minus4
            else if (utmp == 1) {
                utmp = u_n(pdat, 1, &bit_offset);    // delta_pic_order_always_zero_flag
                itmp = se_v(pdat, &bit_offset);        // offset_for_non_ref_pic
                itmp = se_v(pdat, &bit_offset);        // offset_for_non_ref_pic
                utmp = ue_v(pdat, &bit_offset);        // num_ref_frames_in_pic_order_cnt_cycle
                for (j = 0; j < (int) utmp; j++)
                    se_v(pdat, &bit_offset);
            }
            utmp = ue_v(pdat, &bit_offset);            // num_ref_frames
            utmp = u_n(pdat, 1, &bit_offset);        // gaps_in_frame_num_value_allowed_flag

            // Picture Width
            utmp = ue_v(pdat, &bit_offset);            // pic_width_in_mbs_minus1
            *width = ((unsigned int) (utmp + 1)) << 4;    // width = (pic_width_in_mbs_minus1 + 1) * 16

            // Picture Height
            utmp  = ue_v(pdat, &bit_offset);        // pic_height_in_map_units_minus1
            utmp2 = u_n(pdat, 1, &bit_offset);        // frame_mbs_only_flag
            frame_mbs_only_flag = utmp2;            // frame_mbs_only_flag

            PicHeightInMapUnits = utmp + 1;
            FrameHeightInMbs    = (2 - utmp2) * PicHeightInMapUnits;
            PicHeightInMbs      = FrameHeightInMbs;// / (1 + field_pic_flag);
            *height             = ((unsigned int) PicHeightInMbs) << 4;


            // 
            if (!frame_mbs_only_flag)
                utmp = u_n(pdat, 1, &bit_offset);    // mb_adaptive_frame_field_flag
            utmp = u_n(pdat, 1, &bit_offset);        // direct_8x8_inference_flag
            utmp = u_n(pdat, 1, &bit_offset);        // frame_cropping_flag
            if (utmp) {
                utmp = ue_v(pdat, &bit_offset);        // frame_crop_left_offset
                utmp = ue_v(pdat, &bit_offset);        // frame_crop_right_offset
                utmp = ue_v(pdat, &bit_offset);        // frame_crop_top_offset
                utmp = ue_v(pdat, &bit_offset);        // frame_crop_bottom_offset

                *height = (*height - utmp);
            }



            loop_exit = 1;
            break;

        default:
            break;
        }

        pdat++;
    } while (!loop_exit);


    return 1;
}





int get_mpeg4_info(unsigned char *vos, int vos_leng, int *profile, int *width, int *height)
{
    int                loop_exit = 0;
    unsigned int       delimiter;
    unsigned char     *pdat, *p_vos_end;


    int                bit_offset;
    int                nbits;
    unsigned char      utmp;

    unsigned int       video_object_layer_shape, vop_time_increment_resolution;


    pdat       = vos;
    p_vos_end  = vos + vos_leng;
    do {

        delimiter = pdat[0];
        delimiter = (delimiter << 8) | pdat[1];
        delimiter = (delimiter << 8) | pdat[2];
        delimiter &= 0x00FFFFFF;
        pdat += 3;


        while (pdat != p_vos_end) {
            if (delimiter == 0x000001)
                break;

            delimiter = ((delimiter << 8) | *pdat) & 0x00FFFFFF;
            pdat++;
        }

        switch (*pdat) {
        case 0xB0:        // VOS (Video Object Sequence)
            *profile = *(pdat + 1);
            break;

        case 0xB5:
            break;        // VO (Visual Object)

        case 0x00:        // MPEG4 Header Video Object
            break;

        default:
            if ((*pdat & 0xF0) == 0x20) {    // Video Object Layer
    //            ...

                pdat++;
                bit_offset = 0;


                utmp = read_bits(pdat, 1, &bit_offset);        // random_accessible_vol
                utmp = read_bits(pdat, 8, &bit_offset);        // video_object_type_indication
                utmp = read_bits(pdat, 1, &bit_offset);        // is_object_layer_identifier
                if (utmp) {
                    utmp = read_bits(pdat, 4, &bit_offset);        // video_object_layer_verid
                    utmp = read_bits(pdat, 3, &bit_offset);        // video_object_layer_priority
                }
                utmp = read_bits(pdat, 4, &bit_offset);        // aspect_ratio_info
                if (utmp == 15) {    // extended PAR
                    utmp = read_bits(pdat, 8, &bit_offset);        // par_width
                    utmp = read_bits(pdat, 8, &bit_offset);        // par_width
                }
                utmp = read_bits(pdat, 1, &bit_offset);        // vol_control_parameters
                if (utmp) {
                    bit_offset += 3;    // chroma_format(2) & low_delay(1)
                    utmp = read_bits(pdat, 1, &bit_offset);        // vbv_parameters
                    if (utmp) {
                        bit_offset += 63;
                    }
                }

                video_object_layer_shape = read_bits(pdat, 2, &bit_offset);        // video_object_layer_shape


                utmp = read_bits(pdat, 1, &bit_offset);        // marker_bit
                vop_time_increment_resolution = read_bits(pdat, 16, &bit_offset);    // vop_time_increment_resolution
                utmp = read_bits(pdat, 1, &bit_offset);        // marker_bit
                utmp = read_bits(pdat, 1, &bit_offset);        // fixed_vop_rate
                if (utmp) {
                    nbits = num_bits(vop_time_increment_resolution);
                    utmp  = read_bits(pdat, nbits, &bit_offset);    // fixed_vop_time_increment
                }

                if (video_object_layer_shape == 0) {
                    utmp = read_bits(pdat, 1, &bit_offset);        // marker_bit
                    *width   = read_bits(pdat, 13, &bit_offset);    // video_object_layer_width
                    utmp = read_bits(pdat, 1, &bit_offset);        // marker_bit
                    *height  = read_bits(pdat, 13, &bit_offset);    // video_object_layer_height
                    utmp = read_bits(pdat, 1, &bit_offset);        // marker_bit
                }
                utmp = read_bits(pdat, 1, &bit_offset);            // interlaced



                loop_exit = 1;
            }
            break;
        }

        pdat++;
    } while (!loop_exit);


    return 1;
}


int get_h263_info(unsigned char *frame, int frame_leng, int *profile, int *width, int *height)
{

    int            bit_offset;
    unsigned int   utmp;

    unsigned int   type_info;


    bit_offset = 22;
    utmp = read_bits(frame, 8, &bit_offset);    // Temporal Reference
    bit_offset += 5;                            // Type Information (Bit 1 ~ 5)
    type_info = read_bits(frame, 3, &bit_offset);    // Type Information (Bit 6 ~ 8) :  Source Format
    switch (type_info) {
    case 0x1:    // sub-QCIF
        *width  = 88;
        *height = 72;
        break;
    case 0x2:    // QCIF
        *width  = 176;
        *height = 144;
        break;
    case 0x3:    // CIF
        *width  = 352;
        *height = 288;
        break;
    case 0x4:    // 4CIF
        *width  = 704;
        *height = 576;
        break;
    case 0x5:    // 16CIF
        *width  = 1408;
        *height = 1152;
        break;
    case 0x7:    // extended PTYPE

        break;
    case 0x0:    // forbidden
    case 0x6:    // reserved
        return -1;
    default:
        return -1;
    }

    if (type_info == 0x7) {
        // UFEP (3 bits)
        utmp = read_bits(frame, 3, &bit_offset);

        // OPPTYPE(18 bits) if UFEP == 001
        if (utmp == 1) {
            // Source Format (3 bits)
            utmp = read_bits(frame, 3, &bit_offset);
            switch (utmp) {
            case 0x1:    // sub-QCIF
                *width  = 88;
                *height = 72;
                break;
            case 0x2:    // QCIF
                *width  = 176;
                *height = 144;
                break;
            case 0x3:    // CIF
                *width  = 352;
                *height = 288;
                break;
            case 0x4:    // 4CIF
                *width  = 704;
                *height = 576;
                break;
            case 0x5:    // 16CIF
                *width  = 1408;
                *height = 1152;
                break;
            case 0x0:    // forbidden
            case 0x7:    // reserved
                *width  = 0;
                *height = 0;
                break;
            case 0x6:    // custom source format
                *width  = 0;
                *height = 0;
            }
            // Optional custom PCF (1 bit)
            utmp = read_bits(frame, 1, &bit_offset);
            // Optional Unrestricted Motion Vector (UMV) mode (1 bit) : Annex D
            utmp = read_bits(frame, 1, &bit_offset);
            // Optional Syntax-based Arithmetic Coding (SAC) mode (1 bit) : Annex E
            utmp = read_bits(frame, 1, &bit_offset);
            // Optional Advanced Prediction (AP) mode (see Annex F) (1 bit)
            utmp = read_bits(frame, 1, &bit_offset);
            // Optional Advanced INTRA Coding (AIC) mode (Annex I) (1 bit)
            utmp = read_bits(frame, 1, &bit_offset);
            // Optional Deblocking Filter (DF) mode (see Annex J) (1 bit)
            utmp = read_bits(frame, 1, &bit_offset);
            // Optional Slice Structured (SS) mode (see Annex K), (1 bit)
            utmp = read_bits(frame, 1, &bit_offset);
            // Optional Reference Picture Selection (RPS) mode (see Annex N), (1 bit)
            utmp = read_bits(frame, 1, &bit_offset);
            // Optional Independent Segment Decoding (ISD) mode (see Annex R), (1 bit)
            utmp = read_bits(frame, 1, &bit_offset);
            // Optional Alternative INTER VLC (AIV) mode (see Annex S), (1 bit)
            utmp = read_bits(frame, 1, &bit_offset);
            // Optional Modified Quantization (MQ) mode (see Annex T), (1 bit)
            utmp = read_bits(frame, 1, &bit_offset);

            // Bit 15-18 (no meaning)
            bit_offset += 4;
        }

        // MPPTYPE(9 bits)

        // Picture Type Code (Bits 1-3)
        //       :  0(I-pic), 1(P-pic), 2(Improved PB-frame)
        //          3(B-pic), 4(EI-pic), 5(EP-pic)
        utmp = read_bits(frame, 3, &bit_offset);
        // Skip (Bits 4-9)
        bit_offset += 6;


        // CPM(1 bit) & PSBI(2 bits)
        utmp = read_bits(frame, 1, &bit_offset);
        if (utmp == 1)    // PSBI exists only if CPM==1
            utmp = read_bits(frame, 2, &bit_offset);


        // CPFMT (23 bits)

        // Bits 1-4 : Pixel Aspect Ratio Code  (1=1:1, 2=CIF 4:3, 3=525 4:3, 4=CIF 16:9, ...)
        utmp = read_bits(frame, 4, &bit_offset);
        // Bits 5-13 : Picture Width Indication (Real width = (PWI + 1) * 4
        utmp = read_bits(frame, 9, &bit_offset);
        // Bit 14 : 1
        bit_offset += 1;
        // Bits 15-23 : Picture Height Indication (Real Height = PHI * 4
        utmp = read_bits(frame, 9, &bit_offset);
    }



    *profile = 0;

    return 0;
}
