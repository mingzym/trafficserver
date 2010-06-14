#ifndef __UF_CONF_H__
#define __UF_CONF_H__

#include <string>
#include <vector>
#include <ext/hash_map>

using namespace std;
namespace std { using namespace __gnu_cxx; }

namespace __gnu_cxx
{
    template<> struct hash< std::string >
    {
        size_t operator()( const std::string& x ) const
            {
                return hash< const char* >()( x.c_str() );
            }
    };
}

/** Base class for conf value type 
 *  The sole purpose of this class is to act as a base class for conf value types
 */
class ConfValueBase
{
public:
    virtual void dump(ostream &output)=0;
    virtual ~ConfValueBase() { }
};

/** Template conf value 
 *  This covers all built in types.
 *  This is used by UFConf for string, int, bool and double
 *  For specialized conf value types, derive from ConfValueBase
 */
template <class T>
class ConfValue : public ConfValueBase {
public:
    T mElement;
    void dump(ostream& output) { output << mElement; }
    friend ostream& operator <<  (ostream& output, const ConfValue<T>& value)
    {
        output << value.mElement;
        return output;
    }
};

/** Holds config data for a given file
 *  The class has a parent conf variable.
 *  the get* functions look at _data. If the requested key is not found, they lookup the parent
 */
class UFConf
{
public:
    UFConf() : _parent(NULL) { }
    ConfValueBase *get(const std::string &key);
    int *getInt(const std::string &key);
    double *getDouble(const std::string &key);
    string *getString(const std::string &key);
    bool *getBool(const std::string &key);

    void setInt(const std::string &key, int value);
    void setDouble(const std::string &key, double value);
    void setString(const std::string &key, const std::string &value);
    void setBool(const std::string &key, bool value);
    
    void setParent(UFConf *parent) { _parent = parent; }
    bool parse(const std::string &conf_file);
    ~UFConf();

    friend ostream& operator<<(ostream& output, const UFConf &conf);
    
private:
    UFConf *_parent;
    std::hash_map<std::string, ConfValueBase *> _data;
};

/** Manages config objects in the system 
 * 
 */
class UFConfManager
{
public:
    static UFConf* addChildConf(const std::string &conf_file, const std::string &parent_conf_file="/home/y/conf/UF/uf.conf");
    static UFConf* addConf(const std::string &conf_file);
    static UFConf* getConf(const std::string &conf_file);
    static void dump();
private:
    static std::hash_map<std::string, UFConf *> _configs;
};

#endif
