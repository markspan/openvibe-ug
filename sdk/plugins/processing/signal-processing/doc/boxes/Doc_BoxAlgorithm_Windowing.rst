.. _Doc_BoxAlgorithm_Windowing:

Windowing
=========

.. container:: attribution

   :Author:
      Laurent Bonnet
   :Company:
      Mensia Technologies


.. image:: images/Doc_BoxAlgorithm_Windowing.png

Applies a windowing function to the signal.

This plugin is used to apply a window to the input signal.
This plugin allows the selection of the kind of window
(None, Hamming, Hanning, Hann, Blackman, Triangular, Square Root).

Inputs
------

.. csv-table::
   :header: "Input Name", "Stream Type"

   "Input signal", "Signal"

Outputs
-------

.. csv-table::
   :header: "Output Name", "Stream Type"

   "Output signal", "Signal"

.. _Doc_BoxAlgorithm_Windowing_Settings:

Settings
--------

.. csv-table::
   :header: "Setting Name", "Type", "Default Value"

   "Window method", "Window method", "Hamming"

Window method
~~~~~~~~~~~~~

Select the name of window between: None(equivalent to a rectangular window), Hamming, Hanning, Hann, Blackman, 
Triangular and Square Root.

.. _Doc_BoxAlgorithm_Windowing_Examples:

Examples
--------

Let's consider our input signal.
To prevent rebound in spectrum analysis due to the square root
windowing, select a Hanning window for example.

