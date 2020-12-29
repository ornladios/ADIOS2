#ifndef __H5VOL_LINK_FUNC
#define __H5VOL_LINK_FUNC

#include "H5Vol_def.h"

herr_t H5VL_adios2_link_specific(void *obj, const H5VL_loc_params_t *loc_params,
                                 H5VL_link_specific_t specific_type,
                                 hid_t H5_ATTR_UNUSED dxpl_id,
                                 void H5_ATTR_UNUSED **req, va_list arguments)

{
    REQUIRE_NOT_NULL_ERR(loc_params, -1);
    REQUIRE_NOT_NULL_ERR(obj, -1);

    H5VL_ObjDef_t *vol = (H5VL_ObjDef_t *)obj;

    switch (specific_type)
    {
    case H5VL_LINK_EXISTS:
    {
        if ((GROUP == vol->m_ObjType) || (ROOT == vol->m_ObjType))
        {
            htri_t *ret = va_arg(arguments, htri_t *);

            const char *obj_name = loc_params->loc_data.loc_by_name.name;
            *ret = gExistsUnderGrp(vol, obj_name);
        }
        return 0;
    }

    case H5VL_LINK_DELETE:
    {
        ADIOS_VOL_WARN(
            "link does not have effect if already written in file ..\n");

        if ((GROUP == vol->m_ObjType) || (ROOT == vol->m_ObjType))
        {
            if (loc_params->type == H5VL_OBJECT_BY_NAME)
            {
                const char *obj_name = loc_params->loc_data.loc_by_name.name;
                if (gRemoveUnderGrp(vol, obj_name))
                {
                    return 0;
                }
            }
        }
    }
    default:
        break;
    }
    return -1;
}

herr_t H5VL_adios2_link_get(void *obj, const H5VL_loc_params_t *loc_params,
                            H5VL_link_get_t get_type,
                            hid_t H5_ATTR_UNUSED dxpl_id,
                            void H5_ATTR_UNUSED **req, va_list arguments)
{

    REQUIRE_NOT_NULL_ERR(loc_params, -1);
    REQUIRE_NOT_NULL_ERR(obj, -1);

    H5VL_ObjDef_t *vol = (H5VL_ObjDef_t *)obj;
    switch (get_type)
    {
    case H5VL_LINK_GET_NAME:
    {
        char *name = va_arg(arguments, char *);
        size_t size = va_arg(arguments, size_t);
        ssize_t *ret = va_arg(arguments, ssize_t *);

        if ((GROUP == vol->m_ObjType) || (ROOT == vol->m_ObjType))
        {
            // so idx makes sense
            *ret =
                gGetNameOfNthItem(vol, loc_params->loc_data.loc_by_idx.n, name);
            return 0;
        }
        break;
    }

    default:
        break;
    }
    return -1;
}
#endif
