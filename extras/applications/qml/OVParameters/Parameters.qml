import QtQuick
import QtQuick.Controls

import "." as OV

Item {

    id: _self

    property var parameters: []
    property bool displayNames: true
    property bool displayInline: false
    property alias params_model: params_model

    Component {
        id: _dummy_component
        Text {text: lparam.name}
    }

    Component {
        id: _bool_component
        OV.BoolParameter {param: lparam; displayName: displayNames}
    }

    Component {
        id: _color_component
        OV.ColorParameter {param: lparam; displayName: displayNames}
    }

    Component {
        id: _colorgradient_component
        OV.ColorGradientParameter {param: lparam; displayName: displayNames}
    }

    Component {
        id: _list_component
        OV.ListParameter {param: lparam; displayName: displayNames}
    }

    Component {
        id: _multiselect_component
        OV.MultiSelectParameter {param: lparam; displayName: displayNames}
    }

    Component {
        id: _num_component
        OV.NumericParameter {param: lparam; displayName: displayNames}
    }

    Component {
        id: _path_component
        OV.PathParameter {param: lparam; displayName: displayNames}
    }

    Component {
        id: _string_component
        OV.StringParameter {param: lparam; displayName: displayNames; displayInline: _self.displayInline}
    }
/*

    Component {
        id: _range_component
        OV.RangeParameter {param: lparam; decimals: 2, displayName: displayNames}
    }

    Component {
        id: _colormap_component
        OV.Colormap {param: lparam; displayName: displayNames}
    }

    Component {
        id: _lookuptable_component
        OV.LookupTable {param: lparam; displayName: displayNames}
    }

    Component {
        id: _colortable_component
        OV.ColorTableParameter {param: lparam; displayName: displayNames}
    }

    Component {
        id: _nurbs_component
        OV.NurbsParameter {param: lparam; displayName: displayNames}
    }

    Component {
        id: _graphical_component
        OV.GraphicalParameter {param: lparam; displayName: displayNames}
    }
*/
    ListModel {
        id: params_model
        dynamicRoles: true
    }

    function getComponent(type) {
        type = type.replace(',void', '')
        if(type ===  "bool") {
            return _bool_component
        }
        if(type ===  "color") {
            return _color_component
        }
        if(type === "colorgradient") {
            return _colorgradient_component
        }
        if (type === "double") {
            return _num_component
        }
        if(type ===  "int") {
            return _num_component
        }
        if(type === "path") {
            return _path_component
        }
        if(type ===  "string") {
            return _string_component
        }
        if (type === "stringlist" || type === "intlist") {
            return _list_component
        }
        if (type === "multiselect") {
            return _multiselect_component
        }

        return _dummy_component
    }

    function updateParametersModel() {
        params_model.clear()
        for (var p of _self.parameters ?? []) {
            params_model.append({"component": _self.getComponent(p.type), "param": p} )
        }
    }

    onParametersChanged: {
        updateParametersModel()
    }
}
