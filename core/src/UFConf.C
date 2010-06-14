#include "UFConf.H"

#include <fstream>
#include <iostream>
#include <sstream>

/** set string value in conf
 *  Converts the string value that is passed in to a ConfValue and stores it in the conf hash
 */
void UFConf::setString(const string &type, const string &value)
{
    ConfValueBase *existingValue = get(type);
    if(existingValue != NULL) {
        delete existingValue;
    }
    ConfValue<string> *sharedString = new ConfValue<string>;
    sharedString->mElement = value;
    _data[type] = sharedString;
}

/** set int value in conf
 *  Converts the int value that is passed in to a ConfValue and stores it in the conf hash
 */
void UFConf::setInt(const string &type, int value)
{
    ConfValueBase *existingValue = get(type);
    if(existingValue != NULL) {
        delete existingValue;
    }
    ConfValue<int> *sharedInt = new ConfValue<int>;
    sharedInt->mElement = value;
    _data[type] = sharedInt;
}

/** set bool value in conf
 *  Converts the bool value that is passed in to a ConfValue and stores it in the conf hash
 */
void UFConf::setBool(const string &type, bool value)
{
    ConfValueBase *existingValue = get(type);
    if(existingValue != NULL) {
        delete existingValue;
    }
    ConfValue<bool> *sharedBool = new ConfValue<bool>;
    sharedBool->mElement = value;
    _data[type] = sharedBool;
}

/** set double value in conf
 *  Converts double value that is passed in to a ConfValue and stores it in the conf hash
 */
void UFConf::setDouble(const string &type, double value)
{
    ConfValueBase *existingValue = get(type);
    if(existingValue != NULL) {
        delete existingValue;
    }
    ConfValue<double> *sharedDouble = new ConfValue<double>;
    sharedDouble->mElement = value;
    _data[type] = sharedDouble;
}

/** Get the string value associated with the key
 *  Looks at local conf hash for the key
 *  If key is not found in the local conf, forwards request to parent conf
 *  If key is not found in either local or parent conf, NULL is returned
 */
string *UFConf::getString(const string &key)
{
    ConfValue<string> *sharedString = (ConfValue<string> *)get(key);
    if(sharedString != NULL) {
        return &sharedString->mElement;
    }
    if(_parent == NULL)
        return NULL;
    return _parent->getString(key);
}

/** Get the int value associated with the key
 *  Looks at local conf hash for the key
 *  If key is not found in the local conf, forwards request to parent conf
 *  If key is not found in either local or parent conf, NULL is returned
 */
int *UFConf::getInt(const string &key)
{
    ConfValue<int> *sharedInt = (ConfValue<int> *)get(key);
    if(sharedInt != NULL) {
        return &sharedInt->mElement;
    }
    if(_parent == NULL)
        return NULL;
    return _parent->getInt(key);
}

/** Get the bool value associated the with key
 *  Looks at local conf hash for the key
 *  If key is not found in the local conf, forwards request to parent conf
 *  If key is not found in either local or parent conf, NULL is returned
 */
bool *UFConf::getBool(const string &key)
{
    ConfValue<bool> *sharedBool = (ConfValue<bool> *)get(key);
    if(sharedBool != NULL) {
        return &sharedBool->mElement;
    }
    if(_parent == NULL)
        return NULL;
    return _parent->getBool(key);
}

/** Get the double value associated with the key
 *  Looks at local conf hash for the key
 *  If key is not found in the local conf, forwards request to parent conf
 *  If key is not found in either local or parent conf, NULL is returned
 */
double *UFConf::getDouble(const string &type)
{
    ConfValue<double> *sharedDouble = (ConfValue<double> *)get(type);
    if(sharedDouble != NULL) {
        return &sharedDouble->mElement;
    }
    if(_parent == NULL)
        return NULL;
    return _parent->getDouble(type);
}

/** get value associated with the key that is passed in
 *  Looks at local conf hash for the key
 *  If key is not found in the local conf, forwards request to parent conf
 *  If key is not found in either local or parent conf, NULL is returned
 */
ConfValueBase *UFConf::get(const string &key)
{
    hash_map<string, ConfValueBase *>::iterator it = _data.find(key);
    if(it != _data.end())
        return it->second;
    if(_parent == NULL)
        return NULL;
    return _parent->get(key);
}

/** Parse config file and store in conf hash
 *  Looks for config values of type STRING, INT, DOUBLE and BOOL
 *  Skips over lines beginning with '#'
 */
bool UFConf::parse(const std::string &conf_file)
{
    ifstream infile;
    infile.open(conf_file.c_str());
    if(!infile.is_open()) 
        return false; // Could not open file

    string line;
    istringstream instream;
    while(getline(infile, line))
    {
        instream.clear(); // Reset from possible previous errors.
        instream.str(line);  // Use s as source of input.
        string conf_key, conf_key_type;
        if (instream >> conf_key >> conf_key_type) 
        {
            // skip lines starting with #
            if(conf_key[0] == '#')
                continue;

            // get type from config file, read into corresponding value and store  
            string string_value;
            int int_value;
            double double_value;
            bool bool_value;
            if(conf_key_type == "STRING")
            {
                if(instream >> string_value)
                    setString(conf_key, string_value);
            }
            if(conf_key_type == "INT")
            {
                if(instream >> int_value)
                    setInt(conf_key, int_value);
            }
            if(conf_key_type == "DOUBLE")
            {
                if(instream >> double_value)
                    setDouble(conf_key, double_value);
            }
            if(conf_key_type == "BOOL")
            {
                if(instream >> bool_value)
                    setBool(conf_key, bool_value);
            }
        }
    }

    infile.close();
    return true;
}

/**
 *  Dump out config
 */
ostream& operator<<(ostream& output, const UFConf &conf)
{

    for(std::hash_map<std::string, ConfValueBase *>::const_iterator it = conf._data.begin();
        it != conf._data.end();
        it++)
    {
        cerr << it->first << " ";
        it->second->dump(cerr);
        cerr << endl;
    }
    return output;
}

std::hash_map<std::string, UFConf *> UFConfManager::_configs;

/** Add new child conf
 *  Create new conf and set parent to conf object corresponding to the parent_conf that is passed in
 */
UFConf* UFConfManager::addChildConf(const string &conf_file, const string &parent_conf_file)
{
    hash_map<string, UFConf*>::iterator it = _configs.find(conf_file);
    if(it != _configs.end())
    {
        // Conf already exists
        return it->second;
    }

    // Check if parent config was created
    it = _configs.find(parent_conf_file);
    if(it == _configs.end())
        return NULL; // Parent config was not created

    // Create conf
    UFConf *conf_created = addConf(conf_file);

    // Set parent conf
    if(conf_created != NULL)
        conf_created->setParent(it->second);
    return conf_created;
}

/** Add new conf
 *  Creates new conf, sets parent if 'parent' key is present and stores in the conf in the config system
 */
UFConf* UFConfManager::addConf(const string &conf_file)
{
    hash_map<string, UFConf*>::iterator it = _configs.find(conf_file);
    if(it != _configs.end())
    {
        // Conf already exists
        return it->second;
    }

    // Create new UFConf
    UFConf *conf = new UFConf;

    // Parse default config
    string conf_file_default = conf_file + ".default";
    conf->parse(conf_file_default);
    
    // Parse overrides
    conf->parse(conf_file);

    string *conf_file_parent = conf->getString("parent");
    if(conf_file_parent != NULL) 
    {
        conf->setParent(getConf(*conf_file_parent));
    }

    // Store in conf map
    _configs[conf_file] = conf;

    return conf;
}

/**
 *  Get config object pointer associated with the conf file that is passed in
 */
UFConf* UFConfManager::getConf(const string &conf_file)
{
    hash_map<string, UFConf *>::iterator it = _configs.find(conf_file);
    if(it == _configs.end())
        return NULL; // config was not created
    return it->second;
}

/**
 *  Print out all configs in the system to cerr
 */
void UFConfManager::dump()
{
    for(hash_map<string, UFConf *>::iterator it = _configs.begin();
        it != _configs.end();
        it++)
    {
        cerr << "=============CONF " << it->first << " STARTS" << endl;
        cerr << *(it->second);
        cerr << "=============CONF " << it->first << " ENDS" << endl;
    }
}
