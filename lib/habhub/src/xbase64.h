void build_decoding_table(  );
char *xbase64_encode( const char *data, size_t input_length,
                     size_t * output_length, char *encoded_data );
char *xbase64_decode( const char *data, size_t input_length,
                     size_t * output_length );
void xbase64_cleanup(  );
