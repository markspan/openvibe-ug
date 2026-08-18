.. _Doc_BoxAlgorithm_SignalAverage:

Signal average
==============

.. container:: attribution

   :Author:
      Bruno Renier
   :Company:
      Inria/IRISA

.. image:: images/Doc_BoxAlgorithm_SignalAverage.png

This plugin computes the average of each incoming sample
buffer and outputs the resulting signal.

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

   "Filtered signal", "Signal"

Filtered signal
~~~~~~~~~~~~~~~

Signal containing the averages of the input sample buffers.

.. _Doc_BoxAlgorithm_SignalAverage_Miscellaneous:

Miscellaneous
-------------

The output signal's sample count per channel per buffer is one,
since a buffer contains the averages (per channel) of the values
of an input buffer.

