#include "World/Gen.hpp"

void GenDesc::add_pass(Ref<GenPass> pass)
{
    pass->init(*this);
    m_passes.push_back(pass);
}
