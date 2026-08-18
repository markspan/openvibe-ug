.. _Doc_BoxAlgorithm_ContinuousXYZPlot:

Continuous XYZ Plot
===================

.. container:: attribution

   :Author:
      Yann Renard
   :Company:
      Mensia Technologies SA

.. image:: images/Doc_BoxAlgorithm_ContinuousXYZPlot.png

The *Continuous XYZ Plot* displays temporal numerical data in a 2D (*resp.* 3D) space, consecutive rows of the input matrix being grouped by 2 (*resp.* 3) to form the 2D (*resp.* 3D) trajectories.
The display is done **continuously**, meaning that trajectories are persitent along time. Moreover, colors change as a function of time (samples order).

The *Continuous XYZ Plot* box shares common concepts and settings with the other boxes in the **Mensia Advanced Visualization Toolset**.
Additional information are available in the dedicated documentation pages:

- :ref:`Doc_Mensia_AdvViz_Concepts`
- :ref:`Doc_Mensia_AdvViz_Configuration`



Inputs
------

.. csv-table::
   :header: "Input Name", "Stream Type"

   "Matrix", "Signal"

Matrix
~~~~~~

The first input can be a streamed matrix or any derived stream (Signal, Spectrum, Feature Vector).
Please set the input type according to the actual stream type connected.

.. _Doc_BoxAlgorithm_ContinuousXYZPlot_Settings:

Settings
--------

.. csv-table::
   :header: "Setting Name", "Type", "Default Value"

   "Temporal Coherence", "Temporal Coherence", "Time Locked"
   "Time Scale", "Float", "20"
   "Matrix Count", "Integer", "50"
   "Gain", "Float", "1"
   "Caption", "String", ""
   "Translucency", "Float", "1"
   "Show Axis", "Boolean", "true"
   "Use third channel as depth", "Boolean", "false"
   "Color", "Color Gradient", "${AdvancedViz_DefaultColorGradient}"

Temporal Coherence
~~~~~~~~~~~~~~~~~~

Select *Time Locked* for a continuous data stream, and specify the *time scale* below.
Select *Independent* for a discontinuous data stream, and specify the *matrix count* below.

Time Scale
~~~~~~~~~~

The time scale in seconds, before the displays goes back to the origin.

Matrix Count
~~~~~~~~~~~~

The number of input matrices to receive before the displays goes back to the origin.

Gain
~~~~

Gain (floating-point scalar factor) to apply to the input values before display.

Caption
~~~~~~~

Label to be displayed on top of the visualization window.

Translucency
~~~~~~~~~~~~

This setting expect a value between 0 and 1, from transparent to opaque color rendering (nb: this value is the alpha component of the color).

Show Axis
~~~~~~~~~

If this checkbox is ticked, the axis and the grid are displayed.

Use third channel as depth
~~~~~~~~~~~~~~~~~~~~~~~~~~

If this checkbox is ticked, trajectories are plotted in a 3D space, otherwise in a 2D space.

Color
~~~~~

Color gradient to use. This setting can be set manually using the color gradient editor.
Several presets exist in form of configuration tokens ``${AdvancedViz_ColorGradient_X}``, where X can be:

- ``Matlab`` or ``Matlab_Discrete``
- ``Icon`` or ``Icon_Discrete``
- ``Elan`` or ``Elan_Discrete``
- ``Fire`` or ``Fire_Discrete``
- ``IceAndFire`` or ``IceAndFire_Discrete``


The default values ``AdvancedViz_DefaultColorGradient`` or ``AdvancedViz_DefaultColorGradient_Discrete`` are equal to </t>Matlab</tt> and ``Matlab_Discrete``.

.. _Doc_BoxAlgorithm_ContinuousXYZPlot_VizSettings:

Visualization Settings
----------------------

At runtime, all the advanced visualization shared settings are exposed, as described in :ref:`Doc_Mensia_AdvViz_Configuration_RuntimeToolbar`.

