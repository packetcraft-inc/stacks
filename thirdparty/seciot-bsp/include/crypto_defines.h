#ifndef CRYPTO_DEFINES_INCLUDED
#define CRYPTO_DEFINES_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

// Commonly used variables
#define byte_length 8
#define address_bus_length 64
#define bits_64 0x0000000000000000


// Little endian printing or big endian printing for print_uchar8
#define print_little_endian 0
#define print_big_endian 1


// Register addresses
// RSA
#define rsa_base_address 0x0000000005000000
#define rsa_input_text 0x00
#define rsa_exp_text 0x20
#define rsa_mod_text 0x40
#define rsa_r2_mod_n_text 0x60
#define rsa_output_reg 0x80
#define rsa_status_reg 0xc0
// SHA
#define sha256_base_address 0x0000000003000000
#define sha256_input_text_address 0x00
#define sha256_output_reg_address 0x80
#define sha256_config_reg_address 0xc0
#define sha256_status_reg_address 0xc1
// AES
#define aes_base_address 0x0000000004000000
#define aes_input_reg 0x00
#define aes_key_reg 0x20
#define aes_output_reg 0x40
#define aes_config_reg 0x60
#define aes_status_reg 0x61
#define aes_iv_reg 0x50
#define aes_nextblock_reg 0x70


// Error messages
#define error_calloc_memory_unavailable "Memory not allocated.\n"
// RSA
#define rsa_error_message_too_long "Error : Input text, exp_text, and mod_text should be less than 2048 bits.\n"
#define rsa_error_mod_cannot_be_lte_byte "Error : Mod length cannot be less than 8 bits.\n"
#define rsa_error_input_messsage_too_long "Error : Message too long\n"
#define rsa_error_integer_too_large "Error : Integer too large"
#define rsa_error_message_rsa_odd_mode "Error : Mod text should be Odd.\n"
#define rsa_error_memory_unavailable "Error : Memory unavailable. %s\n"
#define rsa_error_decryption_error "Error : Decryption error.\n"
// SHA
#define sha_error_message_input_length "Error : Length of a block is not 512.\n"
#define sha_error_message_input_length_multishot "Error : Length of input should be between 0 and 512. \n"
// AES
#define aes_error_message_only_128bitx_input "Error : The input should contain multiples of 128 bits. In other words multiples of 16 characters. Ensure the string is null appended.\n"
#define aes_error_message_only_spefic_keylengths "Error : The key should have a length of 128, 192, or 256 bits. Ensure the string is null appended.\n"
#define aes_error_message_incorrect_mode "Error : The mode should be inclusively between 0 and 4.\n"


// RSA Padding type
#define RSA_NULL_PAD 0
#define RSAES_PKCS1_v1_5_PAD 1
#define RSAES_OAEP_PAD 2


// RSA Encryption mode
#define rsa_encrypt_mode 1
#define rsa_decrypt_mode 0

#ifdef __cplusplus
}
#endif

#endif
