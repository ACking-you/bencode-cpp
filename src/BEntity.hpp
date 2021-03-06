//
// Created by Alone on 2022-5-7.
//

#pragma once
#include "BObject.h"
#include <functional>
#include <sstream>

//implement BEntity
namespace bencode {
    using LIST = BObject::LIST;
    using DICT = BObject::DICT;
    template<>
    class BEntity<LIST> {
        std::shared_ptr <BObject> object;
        BObject::LIST *list;
    public:
        BEntity() : object(std::make_shared<BObject>(LIST())) {
            list = object->List();
            if (list == nullptr)
                throw std::bad_alloc();
        }

        BEntity &add(BObject src) {
            list->push_back(std::make_shared<BObject>(std::move(src)));
            return *this;
        }

        int bencode(std::ostream &os) {
            return object->Bencode(os);
        }

        friend std::ostream &operator<<(std::ostream &os, const BEntity &entity) {
            entity.object->Bencode(os);
            return os;
        }

        friend std::istream &operator>>(std::istream &is, BEntity &entity) {
            Error error;
            entity.object = std::move(BObject::Parse(is, &error));
            if (error != Error::NoError) {
                perror(error);
                exit(-1);
            }
            return is;
        }
    };

    template<>
    class BEntity<DICT> {
    public:
        std::shared_ptr <BObject> object;
        BObject::DICT *dict{};

        friend class Bencode;

        explicit BEntity(std::shared_ptr <BObject> _object) : object(std::move(_object)) {
            dict = object->Dict();
            if (dict == nullptr)
                throw std::runtime_error("string not a GetDict bencode!");
        }

        BEntity() : object(std::make_shared<BObject>(DICT())) {
            dict = object->Dict();
            if (dict == nullptr)
                throw std::bad_alloc();
        }

        BEntity &put(const std::string &key, BObject value) {
            if (!dict) {
                char msg[200];
                sprintf(msg, "dict nullptr,put GetDict error!\r\n filename %s ,line %d", __FILE__, __LINE__);
                throw std::runtime_error(msg);
            }
            dict->emplace(key, std::make_shared<BObject>(std::move(value)));
            return *this;
        }

        void clear() const {
            if (!dict) {
                char msg[200];
                sprintf(msg, "dict nullptr,clear GetDict error!\r\n filename %s ,line %d", __FILE__, __LINE__);
                throw std::runtime_error(msg);
            }
            dict->clear();
        }

        int bencode(std::ostream &os) {
            return object->Bencode(os);
        }

        //??????json?????????????????????????????????
        std::string to_string() const{
            return object->to_string();
        }

        friend std::ostream &operator<<(std::ostream &os, const BEntity &entity) {
            entity.object->Bencode(os);
            return os;
        }

        friend std::istream &operator>>(std::istream &is, BEntity &entity) {
            Error error;
            entity.object = std::move(BObject::Parse(is, &error));
            if (error != Error::NoError) {
                perror(error);
                exit(-1);
            }
            return is;
        }
    };

    template<>
    class BEntity<int> {
        std::shared_ptr <BObject> object;
        int *val;
    public:
        BEntity() : object(std::make_shared<BObject>(0)) {
            val = object->Int();
            if (val == nullptr)
                throw std::bad_alloc();
        }

        void set(int v) {
            *val = v;
        }

        int *data() {
            return val;
        }

        int bencode(std::ostream &os) {
            return object->Bencode(os);
        }

        friend std::ostream &operator<<(std::ostream &os, const BEntity &entity) {
            entity.object->Bencode(os);
            return os;
        }

        friend BEntity &operator>>(BEntity &b, int &val) {
            if (b.val) {
                val = *b.val;
            } else {
                perror(Error::ErrIvd, "val nullptr!");
            }
            return b;
        }

        friend BEntity &operator<<(BEntity &b, int val) {
            if (b.val) {
                *b.val = val;
            } else {
                perror(Error::ErrIvd, "val nullptr!");
            }
            return b;
        }

        friend std::istream &operator>>(std::istream &is, BEntity &entity) {
            Error error;
            entity.object = std::move(BObject::Parse(is, &error));
            if (error != Error::NoError) {
                perror(error);
                exit(-1);
            }
            return is;
        }
    };

    template<>
    class BEntity<std::string> {
        std::shared_ptr <BObject> object;
        std::string *val;
    public:
        BEntity() : object(std::make_shared<BObject>("")) {
            val = object->Str();
            if (val == nullptr)
                throw std::bad_alloc();
        }

        void set(std::string v) {
            *val = std::move(v);
        }

        const char *data() {
            return val->c_str();
        }

        int bencode(std::ostream &os) {
            return object->Bencode(os);
        }

        friend BEntity &operator<<(BEntity &b, std::string str) {
            if (b.val) {
                *b.val = std::move(str);
            } else {
                perror(Error::ErrIvd, "nullptr b.val");
            }
            return b;
        }

        friend BEntity &operator<<(BEntity &b, const char *str) {
            b << std::string(str);
            return b;
        }

        friend BEntity &operator>>(BEntity &b, std::string &str) {
            if (b.val) {
                str = std::move(*b.val);
            } else {
                perror(Error::ErrIvd, "nullptr b.val");
            }
            return b;
        }

        friend std::ostream &operator<<(std::ostream &os, const BEntity &entity) {
            entity.object->Bencode(os);
            return os;
        }

        friend std::istream &operator>>(std::istream &is, BEntity &entity) {
            Error error;
            entity.object = std::move(BObject::Parse(is, &error));
            if (error != Error::NoError) {
                perror(error);
                exit(-1);
            }
            return is;
        }
    };

    // ????????????
    template<class T>
    extern void to_bencode(Bencode &b, const T &src);

    template<class T>
    extern void from_bencode(Bencode &b, T &src);

    // bencode??????????????????
    class Bencode {
        BEntity<DICT> m_dict;
        std::shared_ptr<BObject> m_list; //????????????append???at(index).value()?????????
        std::string cur_key;

    public:
        Bencode() = default;
        //????????????BObject??????Bencode???
        explicit Bencode(std::shared_ptr<BObject>const& bObject):m_dict(bObject){}
        //putMap && putVector
        template<class T>
        void putMap(BObject &dest, const __DICT__<std::string, T> &src) {
            auto dict = dest.Dict();
            if (!dict) {
                char msg[200];
                sprintf(msg, "object change GetDict error in putMap\r\n filename %s ,line %d", __FILE__, __LINE__);
                throw std::runtime_error(msg);
            }
            for (auto&&[k, v]: src) {
                BObject obj;
                if constexpr(isBasicType<T>::value) {
                    obj = BObject(v);
                } else if constexpr(isMap<T>::value) {
                    obj = std::move(DICT());
                    putMap(obj, v);
                } else if constexpr(isVector<T>::value) {
                    obj = std::move(LIST());
                    putVector(obj, v);
                } else if constexpr(!isBasicType<T>::value) {// ????????????????????????????????????????????????value????????????dict?????????????????????dict???????????????get????????????
                    obj = std::move(DICT());
                    auto new_dict = GetDict(obj);

                    auto pre_dict = m_dict.dict;
                    auto pre_key = cur_key;

                    m_dict.dict = new_dict;
                    to_bencode(*this, v);
                    m_dict.dict = pre_dict;
                    cur_key = pre_key;
                }
                dict->emplace(k, std::make_shared<BObject>(std::move(obj)));
            }
        }

        template<class T>
        void putVector(BObject &dest, const std::vector<T> &src) {
            auto list = dest.List();
            if (!list) {
                char msg[200];
                sprintf(msg, "object change GetList error in putVector\r\n filename %s ,line %d", __FILE__, __LINE__);
                throw std::runtime_error(msg);
            }
            for (auto &&v: src) {
                BObject obj;
                if constexpr(isBasicType<T>::value) {
                    obj = BObject(v);
                } else if constexpr(isMap<T>::value) {
                    obj = std::move(DICT());
                    putMap(obj, v);
                } else if constexpr(isVector<T>::value) {
                    obj = std::move(LIST ());
                    putVector(obj, v);
                } else if constexpr(!isBasicType<T>::value) {// ???????????????????????????getMap?????????????????????????????????????????????cur_key?????????
                    obj = std::move(DICT ());
                    auto new_dict = GetDict(obj);

                    auto pre_dict = m_dict.dict;

                    m_dict.dict = new_dict;
                    to_bencode(*this, v);
                    m_dict.dict = pre_dict;
                }
                list-> emplace_back(std::make_shared<BObject>(std::move(obj)));
            }
        }

        static DICT *GetDict(BObject &src) {
            auto new_dict = src.Dict();
            if (!new_dict) {
                char msg[200];
                sprintf(msg, "GetDict change failed!\r\n filename %s ,line %d", __FILE__, __LINE__);
                throw std::runtime_error(msg);
            }
            return new_dict;
        }


        static LIST *GetList(BObject &src) {
            auto new_list = src.List();
            if (!new_list) {
                char msg[200];
                sprintf(msg, "GetList change failed!\r\n filename %s ,line %d", __FILE__, __LINE__);
                throw std::runtime_error(msg);
            }
            return new_list;
        }

#define APPEND_NAME "LIST"
#define NULL_ERROR(op,type) throw std::runtime_error(#op"() error at:"#type" nullptr");

        //implement append()
        template<class T>
        Bencode& append(const T &src) {
            if (!m_dict.dict) {
                NULL_ERROR(append, GetDict)
            }
            if (!m_list) { //???????????????list????????????????????????
                m_list = std::make_shared<BObject>(LIST());
                m_dict.dict->insert(std::make_pair(APPEND_NAME, m_list));
            }

            auto *pList = GetList(*m_list);  //?????????????????????vector

            auto bobj = std::make_shared<BObject>(); //????????????append???LIST??????
            auto &obj = *bobj;

            //???????????????
            if constexpr(isBasicType<T>::value) {// ????????????????????????????????????????????????
                obj = std::move(BObject(src));
            } else if constexpr(isVector<T>::value) {
                obj = std::move(LIST ());
                putVector(obj, src);
            } else if constexpr(isMap<T>::value) {
                obj = std::move(DICT ());
                putMap(obj, src);
            } else if constexpr(!isBasicType<T>::value) {// ??????????????????????????????????????????????????????????????????????????????????????????????????????????????????dict??????????????????
                obj = std::move(DICT ());
                auto new_dict = GetDict(obj);

                auto old_dict = m_dict.dict;
                m_dict.dict = new_dict;
                to_bencode(*this, src);
                m_dict.dict = old_dict;
            }
            //?????????????????????LIST???????????????BOject??????
            pList-> emplace_back(bobj);
            return *this;
        }
        Bencode& append(const char* src){
            if (!m_dict.dict) {
                NULL_ERROR(append, GetDict)
            }
            if (!m_list) { //???????????????list????????????????????????
                m_list = std::make_shared<BObject>(LIST ());
                m_dict.dict->insert(std::make_pair(APPEND_NAME, m_list));
            }

            auto *pList = GetList(*m_list);  //?????????????????????vector

            auto bobj = std::make_shared<BObject>(); //????????????append???LIST??????
            auto &obj = *bobj;

            obj = std::move(BObject(src));
            pList->emplace_back(bobj);
            return *this;
        }

/**
 * example:
 *      Bencode b;\n
 *      b.append(1).append(2).append("dsafds");\n
 *      b.at(0)<int>.value();\n
 *
 */
        //to help use lamda express
        template<class T>
        struct BValue {
            explicit BValue(std::function<T()> v) : value(std::move(v)) {}

            std::function<T()> value;
        };

        // obtain object by index and use value() to get element
        template<class T>
        BValue<T> at(size_t index) {
            if (!m_dict.dict) {
                NULL_ERROR(at, GetList)
            }
            if (!m_list) { //???????????????????????????????????????????????????????????????????????????
                auto iter = m_dict.dict->find(APPEND_NAME);
                if (iter == m_dict.dict->end()) {
                    throw std::runtime_error("no List exist!!");
                }
                m_list = iter->second; //????????????LIST???????????????
            }
            auto *list = GetList(*m_list);
            auto obj_src = list->at(index); //?????????????????????????????????

            //????????????lamda???????????????????????????????????????obj_src???????????????????????????????????????????????????
            return BValue<T>([src = obj_src,this]() -> T {
                //?????????built-in(????????????)??????????????????BObject???value??????????????????
                if constexpr(isBasicType<T>::value || isVector<T>::value || isMap<T>::value) {
                    return src->value<T>();
                } else { //??????????????????????????????????????????dict???????????????Bencode???dict
                    auto *new_dict = GetDict(*src);
                    auto *old_dict = m_dict.dict;
                    m_dict.dict = new_dict;
                    T ret_value;
                    from_bencode(*this, ret_value);
                    m_dict.dict = old_dict;
                    return ret_value;
                }
            });
        }

        Bencode &operator[](const std::string &key) {
            cur_key = key;
            return *this;
        }

        //operator =
        template<class T>
        Bencode &operator=(const T &src) {
            if (cur_key.empty()) {
                char msg[200];
                sprintf(msg, "operator= valid because of key empty!\r\n filename %s ,line %d", __FILE__, __LINE__);
                throw std::runtime_error(msg);
            }
            BObject obj;
            if constexpr(isBasicType<T>::value) {// ????????????????????????????????????????????????
                obj = std::move(BObject(src));
            } else if constexpr(isVector<T>::value) {
                obj = std::move(LIST ());
                putVector(obj, src);
            } else if constexpr(isMap<T>::value) {
                obj = std::move(DICT ());
                putMap(obj, src);
            } else if constexpr(!isBasicType<T>::value) {// ??????????????????????????????????????????????????????????????????????????????????????????????????????????????????dict??????????????????
                std::string pre_key = cur_key;
                obj = std::move(DICT ());
                auto new_dict = GetDict(obj);

                auto old_dict = m_dict.dict;
                m_dict.dict = new_dict;
                to_bencode(*this, src);
                m_dict.dict = old_dict;
                cur_key = pre_key;
            }
            m_dict.put(cur_key, std::move(obj));
            return *this;
        }


        // getMap && getVector
        template<class T>
        void getMap(__DICT__<std::string, T> &obj, BObject &src) {
            Error error;
            auto dict = src.Dict(&error);
            if (dict == nullptr) {
                char msg[200];
                sprintf(msg, "getMap failed!\r\n filename %s ,line %d", __FILE__, __LINE__);
                throw std::runtime_error(msg);
            }

            for (auto&&[k, v]: *dict) {
                if (!v) {
                    char msg[200];
                    sprintf(msg, "nullptr Exception!\r\n filename %s ,line %d", __FILE__, __LINE__);
                    throw std::runtime_error(msg);
                }
                T tmp;
                BObject &m_data = *v;
                if constexpr(isBasicType<T>::value) {
                    tmp = T(m_data);
                } else if constexpr(isMap<T>::value) {
                    getMap(tmp, m_data);
                } else if constexpr(isVector<T>::value) {
                    getVector(tmp, m_data);
                } else if constexpr(!isBasicType<T>::value) {// ????????????????????????????????????????????????value????????????dict?????????????????????dict???????????????get????????????
                    auto new_dict = GetDict(m_data);
                    auto pre = m_dict.dict;
                    m_dict.dict = new_dict;
                    from_bencode(*this, tmp);
                    m_dict.dict = pre;
                }
                obj.emplace(k, std::move(tmp));
            }
        }

        template<class T>
        void getVector(std::vector<T> &obj, BObject &src) {
            Error error;
            auto list = src.List(&error);
            if (list == nullptr) {
                char msg[200];
                sprintf(msg, "getVector failed!\r\n filename %s ,line %d", __FILE__, __LINE__);
                throw std::runtime_error(msg);
            }
            for (auto &&v: *list) {
                if (!v) {
                    char msg[200];
                    sprintf(msg, "nullptr Exception!\r\n filename %s ,line %d", __FILE__, __LINE__);
                    throw std::runtime_error(msg);
                }
                BObject &m_data = *v;
                T tmp;
                if constexpr(isBasicType<T>::value) {
                    tmp = T(m_data);
                } else if constexpr(isMap<T>::value) {
                    getMap(tmp, m_data);
                } else if constexpr(isVector<T>::value) {
                    getVector(tmp, m_data);
                } else {// ?????????????????????
                    auto new_dict = GetDict(m_data);
                    auto pre = m_dict.dict;
                    m_dict.dict = new_dict;
                    from_bencode(*this, tmp);
                    m_dict.dict = pre;
                }
                obj.emplace_back(std::move(tmp));
            }
        }

        //get_to(T) [to call get<T>() implement this function]
        template<class T>
        void get_to(T &dest) {
            dest = std::move(get<T>());
        }

        template<class T>
        T get() {
            if (!m_dict.dict) {
                char msg[200];
                sprintf(msg, "nullptr Exception!\r\n filename %s ,line %d", __FILE__, __LINE__);
                throw std::runtime_error(msg);
            }
            auto it = m_dict.dict->find(cur_key);
            T ret;
            if (it != m_dict.dict->end()) {
                BObject &object = *it->second;
                if constexpr(isMap<T>::value) {
                    getMap(ret, object);
                } else if constexpr(isVector<T>::value) {
                    getVector(ret, object);
                } else if constexpr(isBasicType<T>::value) {
                    ret = T(object);
                } else if constexpr(!isBasicType<T>::value) {// ??????????????????????????????????????????object?????????dict????????????????????????dict????????????
                    auto new_dict = GetDict(object);
                    auto pre = m_dict.dict;
                    m_dict.dict = new_dict;
                    from_bencode(*this, ret);
                    m_dict.dict = pre;
                }
            } else {
                perror(Error::ErrIvd, "at get<T>(): can't find key!");
                exit(-1);
            }
            return ret;
        }


        // overload operator<<
        template<class T>
        friend Bencode &operator<<(Bencode &bencode, const T &src) {
            bencode.m_dict.clear(); //???????????????????????????
            to_bencode(bencode, src);
            return bencode;
        }

        template<class T>
        friend Bencode &operator<<(Bencode &bencode, __DICT__<std::string, T> const &src) {
            bencode.m_dict.clear(); //???????????????????????????
            BObject &obj = *bencode.m_dict.object;
            bencode.putMap(obj, src);
            return bencode;
        }


        template<class T>
        friend Bencode &operator<<(Bencode &bencode, std::vector<T> const &src) {
            bencode.m_dict.clear(); //???????????????????????????
            BObject list = BObject(LIST());
            bencode.template putVector(list, src);
            bencode.m_dict.put("GetList", list);
            return bencode;
        }

        friend Bencode &operator<<(Bencode &bencode, const std::string &src) {
            bencode.m_dict.clear();
            BObject str = BObject(src);
            bencode.m_dict.put("STR", str);
            return bencode;
        }

        friend Bencode &operator<<(Bencode &bencode, const int &src) {
            bencode.m_dict.clear();
            BObject integer = BObject(src);
            bencode.m_dict.put("INT", integer);
            return bencode;
        }

        friend Bencode &operator<<(Bencode &bencode, const char *src) {
            bencode.m_dict.clear();
            BObject str = BObject(src);
            bencode.m_dict.put("STR", str);
            return bencode;
        }

        // overload operator>>
        template<class T>
        friend Bencode &operator>>(Bencode &bencode, T &src) {
            from_bencode(bencode, src);
            return bencode;
        }

        template<class T>
        friend Bencode &operator>>(Bencode &bencode, __DICT__<std::string, T> &dest) {
            BObject &obj = *bencode.m_dict.object;
            bencode.template getMap(dest, obj);
            return bencode;
        }

        template<class T>
        friend Bencode &operator>>(Bencode &bencode, std::vector<T> &dest) {
            auto dict = bencode.m_dict.dict;
            if (dict) {
                auto ret = dict->find("GetList");
                if (ret != dict->end()) {
                    BObject &p = *ret->second;
                    bencode.template getVector(dest, p);
                } else {
                    perror(Error::ErrTyp, "not find List can convert!");
                }
            } else {
                perror(Error::ErrIvd, "at convert to GetList ,GetDict is nullptr!");
            }
            return bencode;
        }

        friend Bencode &operator>>(Bencode &bencode, std::string &dest) {
            auto dict = bencode.m_dict.dict;
            if (dict) {
                auto ret = dict->find("STR");
                if (ret != dict->end()) {
                    BObject &p = *ret->second;
                    dest = std::string(p);
                } else {
                    perror(Error::ErrTyp, "not find Str can convert!");
                }
            } else {
                perror(Error::ErrIvd, "at convert to Str ,GetDict is nullptr!");
            }
            return bencode;
        }

        friend Bencode &operator>>(Bencode &bencode, int &dest) {
            auto dict = bencode.m_dict.dict;
            if (dict) {
                auto ret = dict->find("INT");
                if (ret != dict->end()) {
                    BObject &p = *ret->second;
                    dest = int(p);
                } else {
                    perror(Error::ErrTyp, "not find Int can convert!");
                }
            } else {
                perror(Error::ErrIvd, "at convert to Int ,GetDict is nullptr!");
            }
            return bencode;
        }


        // overload stream operator<< and operator>>
        friend std::ostream &operator<<(std::ostream &os, Bencode &bencode) {
            bencode.m_dict.bencode(os);
            return os;
        }

        friend std::istream &operator>>(std::istream &is, Bencode &bencode) {
            Error error;
            bencode.m_dict.object = std::move(BObject::Parse(is, &error));

            if (error != Error::NoError) {
                perror(error);
                exit(-1);
            }
            bencode.m_dict.dict = bencode.m_dict.object->Dict(&error);
            if (error != Error::NoError) {
                char msg[200];
                sprintf(msg, "in operator>>,stream not a GetDict!\r\n filename %s ,line %d", __FILE__, __LINE__);
                throw std::runtime_error(msg);
            }
            return is;
        }

        //TODO ?????????to_string???????????????string????????????
        std::string to_string(){
            return m_dict.to_string();
        }
    };
}

