#include <cstdlib>
#include <cstring>

class C {
public:
    static const char* name() {
        return "C";
    }
    static int64_t hash() {
        static std::hash<std::string> hash_func;
        hash_func(name());
    }
    static constexpr int32_t size() {
        return sizeof(uint64_t) +
               sizeof(int32_t) +
               sizeof(int64_t) +
               sizeof(double) +
               sizeof(int32_t) * 4 +
               sizeof(s_);
    }

    uint64_t key() const {return key_;}
    void key(uint64_t key) {key_ = key;}

    void serialize(char* data) const {
        memcpy(data, reinterpret_cast<char*>(const_cast<uint64_t*>(&key_)),
               sizeof(uint64_t));
        data += sizeof(uint64_t);
        memcpy(data, reinterpret_cast<char*>(const_cast<int32_t*>(&i32_)),
               sizeof(int32_t));
        data += sizeof(int32_t);
        memcpy(data, reinterpret_cast<char*>(const_cast<int64_t*>(&i64_)),
               sizeof(int64_t));
        data += sizeof(int64_t);
        memcpy(data, reinterpret_cast<char*>(const_cast<double*>(&d_)),
               sizeof(double));
        data += sizeof(double);
        for (std::size_t i = 0; i < a_.size(); ++i) {
            memcpy(data, reinterpret_cast<char*>(const_cast<int32_t*>(&a_[i])),
                   sizeof(int32_t));
            data += sizeof(int32_t);
        }
        memcpy(data, s_, sizeof(s_));
    }
    void deserialize(const char* data) {
        memcpy(reinterpret_cast<char*>(&key_), data, sizeof(uint64_t));
        data += sizeof(uint64_t);
        memcpy(reinterpret_cast<char*>(&i32_), data, sizeof(int32_t));
        data += sizeof(int32_t);
        memcpy(reinterpret_cast<char*>(&i64_), data, sizeof(int64_t));
        data += sizeof(int64_t);
        memcpy(reinterpret_cast<char*>(&d_), data, sizeof(double));
        data += sizeof(double);
        for (std::size_t i = 0; i < a_.size(); ++i) {
            memcpy(reinterpret_cast<char*>(&a_[i]), data, sizeof(int32_t));
            data += sizeof(int32_t);
        }
        memcpy(s_, data, sizeof(s_));
    }

    void assign_data(uint64_t key, int32_t i32, int64_t i64, 
                     double d, std::array<int32_t, 4>&& a, const char* s) {
        key_ = key;
        i32_ = i32;
        i64_ = i64;
        d_ = d;
        a_ = a;
        memcpy(s_, s, sizeof(s_));
    }

    void print() {
        std::cout << key_ << " "
                  << i32_ << " "
                  << i64_ << " "
                  << d_ << " "
                  << a_[0] << " "
                  << a_[1] << " "
                  << a_[2] << " "
                  << a_[3] << " "
                  << s_ << " "
                  << std::endl;
    }

private:
    uint64_t key_;
    int32_t i32_;
    int64_t i64_;
    double d_;
    std::array<int32_t, 4> a_;
    char s_[1024];
};