#include "mesh_shmdata.h"

#include "log.h"
#include "timer.h"

using namespace std;

namespace Splash {

/*************/
Mesh_Shmdata::Mesh_Shmdata()
{
    _type = "mesh_shmdata";

    registerAttributes();
}

/*************/
Mesh_Shmdata::~Mesh_Shmdata()
{
    if (_reader != nullptr)
        shmdata_any_reader_close(_reader);
}

/*************/
bool Mesh_Shmdata::read(const string& filename)
{
    if (_reader != nullptr)
        shmdata_any_reader_close(_reader);

    _reader = shmdata_any_reader_init();
    shmdata_any_reader_run_gmainloop(_reader, SHMDATA_TRUE);
    shmdata_any_reader_set_on_data_handler(_reader, Mesh_Shmdata::onData, this);
    shmdata_any_reader_start(_reader, filename.c_str());
    _filename = filename;

    return true;
}

/*************/
void Mesh_Shmdata::onData(shmdata_any_reader_t* reader, void* shmbuf, void* data, int data_size, unsigned long long timestamp,
    const char* type_description, void* user_data)
{
    Mesh_Shmdata* ctx = reinterpret_cast<Mesh_Shmdata*>(user_data);

    lock_guard<mutex> lock(ctx->_writeMutex);

    shmdata_any_reader_free(shmbuf);
}

/*************/
void Mesh_Shmdata::registerAttributes()
{
    _attribFunctions["file"] = AttributeFunctor([&](vector<Value> args) {
        if (args.size() < 1)
            return false;
        return read(args[0].asString());
    });
}

} // end of namespace
