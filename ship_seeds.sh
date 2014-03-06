#!/bin/bash

for i in n w s e
do
  ./show_reaction test/31c240_simple.cfg `echo "select emitted.rId from emitted left join reaction using (rid) left join objects using (oId) where result = 'stable' and name like '%$i';" |  mysql -N -s -u gol -p gol` > sample_$i.pat
done
