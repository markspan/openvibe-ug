.. _Doc_BoxAlgorithm_GenericStreamWriter:

Generic stream writer
=====================

.. container:: attribution

   :Author:
      Yann Renard
   :Company:
      INRIA

.. image:: images/Doc_BoxAlgorithm_GenericStreamWriter.png

This box is able to dump any OpenViBE stream into a binary file. In the cacse where this box
would have multiple inputs, the streams would be multiplexed in the file. Such file can
be read back with the :ref:`Doc_BoxAlgorithm_GenericStreamReader`

Inputs
------

.. csv-table::
   :header: "Input Name", "Stream Type"

   "Input Signal", "Signal"
   "Input Stimulations", "Stimulations"

You can add any input you want to this box depending on the number of streams you want to dump.
In the cacse where this box would have multiple inputs, the streams would be multiplexed in the file.

Input Signal
~~~~~~~~~~~~

The default input.

**Note: it important to correctly configure the type of the inputs**. That information will be
used by the :ref:`Doc_BoxAlgorithm_GenericStreamReader` to map the contained streams to its outputs.

.. _Doc_BoxAlgorithm_GenericStreamWriter_Settings:

Settings
--------

.. csv-table::
   :header: "Setting Name", "Type", "Default Value"

   "Filename", "Filename", ""
   "Use compression", "Boolean", "false"

Filename
~~~~~~~~

This setting points to the file to write the streams to.


- Default value : [ *record-[$core{date}-$core{time}].ov* ]


Use compression
~~~~~~~~~~~~~~~

Thanks to this setting, you can use compression on each input stream. This means that the basic
structure of the file remains uncompressed but that each stream inside this structure is compressed.
**Note: this is not implemented at the moment**.

