#pragma once

#include "UI/Widget.hpp"

class TextArea : public Widget
{
    CLASS(TextArea, Widget);

public:
    //

private:
    std::vector<std::shared_ptr<Text>> m_texts;
};
