#include"aes.h"
#include"util.h"
//#include"wmbus.h"

#include<assert.h>
#include<memory.h>
#include<vector>
#include <ArduinoJson.h>

namespace std
{

bool decrypt_TPL_AES_CBC_IV(vector<uchar> &frame,
                            vector<uchar>::iterator &pos,
                            vector<uchar> &key,
                            uchar *iv,
                            int *num_encrypted_bytes,
                            int *num_not_encrypted_at_end)
{
    vector<uchar> buffer;

    std::string s;

    buffer.insert(buffer.end(), pos, frame.end());

    size_t num_bytes_to_decrypt = frame.end()-pos;

    uint8_t tpl_num_encr_blocks = 3; //t->tpl_num_encr_blocks

    if (tpl_num_encr_blocks)
    {
        num_bytes_to_decrypt = tpl_num_encr_blocks*16;
    }

    *num_encrypted_bytes = num_bytes_to_decrypt;
    *num_not_encrypted_at_end = buffer.size()-num_bytes_to_decrypt;

    char cs[100];
    sprintf(cs, "(TPL) num encrypted blocks %zu (%d bytes and remaining unencrypted %zu bytes)\n",
             tpl_num_encr_blocks, num_bytes_to_decrypt, buffer.size()-num_bytes_to_decrypt);
    Serial.print(cs);

    if (key.size() == 0) return false;

//    debugPayload("(TPL) AES CBC IV decrypting", buffer);
//    s = bin2hex(buffer);
//    Serial.print("(TPL) AES CBC IV decrypting "); Serial.println(s.c_str());


    if (buffer.size() < num_bytes_to_decrypt)
    {
//        warning("(TPL) warning: decryption received less bytes than expected for decryption! "
//                "Got %zu bytes but expected at least %zu bytes since num encr blocks was %d.\n",
//                buffer.size(), num_bytes_to_decrypt,
//                tpl_num_encr_blocks);
        sprintf(cs, "(TPL) warning I ... %zu %zu %d.\n",
                    buffer.size(), num_bytes_to_decrypt,  tpl_num_encr_blocks);
        Serial.print(cs);
        num_bytes_to_decrypt = buffer.size();
    }

    // The content should be a multiple of 16 since we are using AES CBC mode.
    if (num_bytes_to_decrypt % 16 != 0)
    {
//        warning("(TPL) warning: decryption received non-multiple of 16 bytes! "
//                "Got %zu bytes shrinking message to %zu bytes.\n",
//                num_bytes_to_decrypt, num_bytes_to_decrypt - num_bytes_to_decrypt % 16);
//        num_bytes_to_decrypt -= num_bytes_to_decrypt % 16;
          sprintf(cs, "(TPL) warning II ... %zu %zu %d.\n",
                  num_bytes_to_decrypt, num_bytes_to_decrypt - num_bytes_to_decrypt % 16);
        Serial.print(cs);
        assert (num_bytes_to_decrypt % 16 == 0);
    }

    std::vector<uchar> ivv(iv, iv+16);
    s = bin2hex(ivv);
    Serial.printf("(TPL) ivv %s\n",s.c_str());
    s = bin2hex(key);
    Serial.printf("(TPL) key %s\n",s.c_str());
//    debug("(TPL) IV %s\n", s.c_str());

    uchar buffer_data[num_bytes_to_decrypt];
    memcpy(buffer_data, safeButUnsafeVectorPtr(buffer), num_bytes_to_decrypt);
    uchar decrypted_data[num_bytes_to_decrypt];
    memcpy(decrypted_data, buffer_data,num_bytes_to_decrypt);

    //AES_CBC_decrypt_buffer(decrypted_data, buffer_data, num_bytes_to_decrypt, safeButUnsafeVectorPtr(aeskey), iv);
    //void AES_CBC_decrypt_buffer(uint8_t* output, uint8_t* input, uint32_t length, const uint8_t* key, const uint8_t* iv)

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, &key[0], iv);
    //void AES_CBC_decrypt_buffer(struct AES_ctx* ctx, uint8_t* buf, size_t length)

    AES_CBC_decrypt_buffer(&ctx, decrypted_data, num_bytes_to_decrypt);


    // Remove the encrypted bytes.
    frame.erase(pos, frame.end());

    // Insert the decrypted bytes.
    //frame.insert(frame.end(), decrypted_data, decrypted_data+num_bytes_to_decrypt);
    frame.insert(frame.end(), decrypted_data, decrypted_data+num_bytes_to_decrypt);


    s = bin2hex(buffer_data, num_bytes_to_decrypt);
    Serial.printf("(TPL) data %s\n", s.c_str());

    s = bin2hex(decrypted_data, num_bytes_to_decrypt);
    Serial.printf("(TPL) decr %s\n", s.c_str());

    if (num_bytes_to_decrypt < buffer.size())
    {
        frame.insert(frame.end(), buffer.begin()+num_bytes_to_decrypt, buffer.end());
        Serial.printf("(TPL) appended ... \n");
    }
    return true;
}


}// namespace std