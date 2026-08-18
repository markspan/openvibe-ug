.. _Doc_BoxAlgorithm_3DCubes:

3D Cubes
========

.. container:: attribution

   :Author:
      Yann Renard
   :Company:
      Mensia Technologies SA

.. image:: images/Doc_BoxAlgorithm_3DCubes.png

The *3D Cubes* box displays the data in a topographic view, where each channel (correctly identified and positionned thanks to a channel localisation file) is associated with a cube.
All the cubes are positionned in a 3D space according to the corresponding positions of the electrodes on the scalp.
The input data is displayed through 2 modalities:

- the **cubes color**
- the **cubes size**


The cube size varies according to the same - absolute - range as the color, i.e. high negative or positive values will be 
displayed as big cube while values close to zero with be displayed as small cubes.

The *3D Cubes* box shares common concepts and settings with the other boxes in the **Mensia Advanced Visualization Toolset**.
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

The box input can be a streamed matrix or any derived stream (Signal, Spectrum, Feature Vector).
Please set the input type according to the actual stream type connected.

.. _Doc_BoxAlgorithm_3DCubes_Settings:

Settings
--------

.. csv-table::
   :header: "Setting Name", "Type", "Default Value"

   "Channel Localisation", "Filename", "${AdvancedViz_ChannelLocalisation}"
   "Gain", "Float", "1"
   "Caption", "String", ""
   "Color", "Color Gradient", "${AdvancedViz_DefaultColorGradient}"

Channel Localisation
~~~~~~~~~~~~~~~~~~~~

The channel localisation file containing the cartesian coordinates of the electrodes to be displayed.
A default configuration file is provided, and its path stored in the configuration token ``${AdvancedViz_ChannelLocalisation}``.

Gain
~~~~

Gain (floating-point scalar factor) to apply to the input values before display.

Caption
~~~~~~~

Label to be displayed on top of the visualization window.

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

An example of topography rendering using these color gradients can be found :ref:`Doc_Mensia_AdvViz_Configuration` "here".

.. _Doc_BoxAlgorithm_3DCubes_VizSettings:

Visualization Settings
----------------------

At runtime, all the advanced visualization shared settings are exposed, as described in :ref:`Doc_Mensia_AdvViz_Configuration_RuntimeToolbar`.
Note that if the box receives a discontinuous data stream, such as a re-epoched signal through stimulation based epoching, the ERP replay features is exposed.
Using the ERP replay allows you to slowly visualize the last epoch received.

.. _Doc_BoxAlgorithm_3DCubes_Examples:

Examples
--------

In the following example, we compute the band power of the signal in the 8-15 Hz frequency range, and average it over the last 32 epochs received.

You can find a commented scenario in the provided sample set, the scenario file name is \textit{3DCubes.xml}.

.. figure:: images/3DCubes_Example.png
   :alt: Example of scenario using the 3D cubes
   :align: center

   Example of scenario using the 3D cubes

