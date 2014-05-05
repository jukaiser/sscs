#!/bin/bash

for i in n w s e
do
  echo ./recipe test/31c240.cfg `echo "select emitted.rId from emitted left join reaction using (rId) left join object using (oId) where result = 'stable' and name like '%$i';" |  mysql -N -s -u gol -p gol` > sample_$i.pat
done
