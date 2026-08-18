.. _Doc_BoxAlgorithm_StreamEndDetector:

Stream End Detector
===================

.. container:: attribution

   :Author:
      Jozef Legeny
   :Company:
      Mensia Technologies


This box waits until it receives an End chunk in the incoming stream and sends
a stimulation once it receives it.


.. _Doc_BoxAlgorithm_StreamEndDetector_Inputs:

Inputs
------

This box receives a single input of arbitrary type derived from EBML. It does
not do any specific decoding.

.. csv-table::
   :header: "Input Name", "Stream Type"

   "EBML Stream", "EBML stream"


.. _Doc_BoxAlgorithm_StreamEndDetector_Outputs:

Outputs
-------

This box will output a single stimulation after receiving the End chunk. Before
outputting the stimulation, the stream will contain empty chunks in sync with
the input stream.

.. csv-table::
   :header: "Output Name", "Stream Type"

   "Output Stimulations", "Stimulations"


.. _Doc_BoxAlgorithm_StreamEndDetector_Settings:

Settings
--------

.. csv-table::
   :header: "Setting Name", "Type", "Default Value"

   "Stimulation name", "Stimulation", "OVTK_StimulationId_Label_00"

.. _Doc_BoxAlgorithm_StreamEndDetector_Setting_1:

Stimulation name
~~~~~~~~~~~~~~~~

Name or identifier of the stimulation to be sent by this box.

.. _Doc_BoxAlgorithm_StreamEndDetector_Examples:

Examples
--------

It can be combined with :ref:`Doc_BoxAlgorithm_PlayerController` to stop a
scenario on file end.


