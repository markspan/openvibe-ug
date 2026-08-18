.. _Doc_BoxAlgorithm_ReferenceChannel:

Reference Channel
=================

.. container:: attribution

   :Author:
      Yann Renard
   :Company:
      Inria

.. image:: images/Doc_BoxAlgorithm_ReferenceChannel.png

Reference channel must be specified as a parameter for the box

This plugin subtracts the values of the samples from a reference
channel from the other channels' samples.

Inputs
------

.. csv-table::
   :header: "Input Name", "Stream Type"

   "Input signal", "Signal"

Input signal
~~~~~~~~~~~~

The input signal.

Outputs
-------

.. csv-table::
   :header: "Output Name", "Stream Type"

   "Output signal", "Signal"

Output signal
~~~~~~~~~~~~~

The resulting signal.

.. _Doc_BoxAlgorithm_ReferenceChannel_Settings:

Settings
--------

.. csv-table::
   :header: "Setting Name", "Type", "Default Value"

   "Channel", "String", "Ref_Nose"
   "Channel Matching Method", "Match method", "Smart"

Channel
~~~~~~~

Index of the reference channel in the input stream.

.. _Doc_BoxAlgorithm_ReferenceChannel_Miscellaneous:

Miscellaneous
-------------

The reference channel is kept in the resulting signal.

