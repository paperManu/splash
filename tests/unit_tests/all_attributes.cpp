#include <doctest.h>

#include "./core/factory.h"
#include "./core/root_object.h"

using namespace Splash;

/*************/
TEST_CASE("Testing attributes for all objects the Factory can create")
{
    auto factory = Factory(nullptr);

    auto objectTypes = factory.getObjectTypes();
    for (const auto& objectType : objectTypes)
    {
        const auto object = factory.create(objectType);
        if (!object)
            continue;

        auto attributeList = object->getAttributesList();

        for (const auto& attribute : attributeList)
        {
            const auto ret = object->getAttribute(attribute);
            if (!ret.has_value())
                continue;

            const auto& value = ret.value();
            if (value.empty())
                continue;

            const auto setRet = object->setAttribute(attribute, value);
            CHECK(setRet != BaseObject::SetAttrStatus::failure);
            if (setRet == BaseObject::SetAttrStatus::no_setter)
                continue;

            const auto retAfterSet = object->getAttribute(attribute);
            const auto& valueAfterSet = retAfterSet.value();

            CHECK_EQ(value, valueAfterSet);
        }
    }
}
