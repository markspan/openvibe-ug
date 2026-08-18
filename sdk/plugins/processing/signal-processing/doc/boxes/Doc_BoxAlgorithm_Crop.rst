.. _Doc_BoxAlgorithm_Crop:

Crop
====

.. container:: attribution

   :Author:
      Yann Renard
   :Company:
      Inria/IRISA

.. image:: images/Doc_BoxAlgorithm_Crop.png

Minimum or maximum or both limits can be specified

This box allows to set minimum and/or maximum thresholds to incoming data. Values lying outside the allowed range are cropped to it.

Inputs
------

.. csv-table::
   :header: "Input Name", "Stream Type"

   "Input matrix", "Streamed matrix"

Outputs
-------

.. csv-table::
   :header: "Output Name", "Stream Type"

   "Output matrix", "Streamed matrix"

.. _Doc_BoxAlgorithm_Crop_Settings:

Settings
--------

.. csv-table::
   :header: "Setting Name", "Type", "Default Value"

   "Crop method", "Crop method", "Min/Max"
   "Min crop value", "Float", "-1"
   "Max crop value", "Float", "1"

Crop method
~~~~~~~~~~~

Method to use to crop incoming data. A minimum and/or a maximum threshold(s) may be defined.

Min crop value
~~~~~~~~~~~~~~

Minimum threshold.

Max crop value
~~~~~~~~~~~~~~

Maximum threshold.

.. _Doc_BoxAlgorithm_Crop_Miscellaneous:

Miscellaneous
-------------

Note : the type of input data may be changed. Allowed types include streamed matrix, feature vector, signal and spectrum.

