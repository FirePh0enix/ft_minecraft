#include "World/Gen.hpp"

void GenDesc::add_pass(Ref<GenPass> pass)
{
    pass->init(*this);
    m_passes.push_back(pass);
}

void GenDesc::add_struct_pass(Ref<StructurePass> pass)
{
    pass->init(*this);
    m_struct_passes.push_back(pass);
}
