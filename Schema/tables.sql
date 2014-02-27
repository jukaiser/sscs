DROP TABLE IF EXISTS bullets;
CREATE TABLE bullets (
  bId int(11) NOT NULL AUTO_INCREMENT PRIMARY KEY,
  name varchar(20) NOT NULL,
  -- rle varchar(30) NOT NULL,
  oId int(11) NOT NULL,
  base_x int(11) NOT NULL,
  base_y int(11) NOT NULL,
  reference enum('TOPLEFT','TOPRIGHT','BOTTOMLEFT','BOTTOMRIGHT') DEFAULT NULL,
  lane_dx int(11) NOT NULL,
  lane_dy int(11) NOT NULL,
  lanes_per_height int(11) NOT NULL,
  lanes_per_width int(11) NOT NULL,
  extra_lanes int(11) NOT NULL,
  UNIQUE KEY (name)
  -- KEY (rle)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

-- INSERT INTO bullets VALUES
	-- (1,'gl-se','o$b2o$2o!',3,3,1,1,4,-4,0,'BOTTOMLEFT',0,-1,1,1,8),
	-- (2,'LWSS-e:0','o2bo$4bo$o3bo$b4o!',5,4,2,0,4,-6,2,'BOTTOMLEFT',0,-1,1,0,7),
	-- (3,'LWSS-e:2','b4o$o3bo$4bo$o2bo!',5,4,2,0,4,-6,2,'BOTTOMLEFT',0,-1,1,0,7);

DROP TABLE IF EXISTS chain;
CREATE TABLE chain (
  cId int(11) NOT NULL AUTO_INCREMENT PRIMARY KEY,
  context varchar(40) NOT NULL,
  parent_tId int(11) DEFAULT NULL,
  parent_cId int(11) DEFAULT NULL,
  rId int(11) DEFAULT NULL,
  cost int(11) NOT NULL,
  KEY (context),
  KEY (parent_tId),
  KEY (parent_cId),
  KEY (cost)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 DATA DIRECTORY = '/home/mysql-gol2' INDEX DIRECTORY = '/home/mysql-gol2';

DROP TABLE IF EXISTS emitted;
CREATE TABLE emitted (
  eId int(11) NOT NULL AUTO_INCREMENT PRIMARY KEY,
  rId int(11) NOT NULL,
  oId int(11) NOT NULL,
  offX int(11) NOT NULL,
  offY int(11) NOT NULL,
  gen int(11) NOT NULL,
  KEY (rId),
  KEY (oId)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 DATA DIRECTORY = '/home/mysql-gol2' INDEX DIRECTORY = '/home/mysql-gol2';

DROP TABLE IF EXISTS reaction;
CREATE TABLE reaction (
  rId int(11) NOT NULL AUTO_INCREMENT PRIMARY KEY,
  initial_tId int(11) NOT NULL,
  bId int(11) NOT NULL,
  lane int(11) NOT NULL,
  result_tId int(11) NOT NULL,
  offX int(11) NOT NULL,
  offY int(11) NOT NULL,
  gen int(11) NOT NULL,
  result enum('dies','fly-by','stable','unfinished', 'explodes','pruned') DEFAULT NULL,
  emits_ships enum('false','true') DEFAULT 'false',
  cost int(11) NOT NULL,
  -- summary varchar(4000) NOT NULL,
  UNIQUE (initial_tId, bId, lane),
  KEY (result),
  KEY (initial_tId),
  KEY (result_tId)
  -- FULLTEXT INDEX (summary)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 DATA DIRECTORY = '/home/mysql-gol2' INDEX DIRECTORY = '/home/mysql-gol2';

DROP TABLE IF EXISTS target;
CREATE TABLE target (
  tId int(11) NOT NULL AUTO_INCREMENT PRIMARY KEY,
  rle varchar(1000) NOT NULL,
  width int(11) NOT NULL,
  height int(11) NOT NULL,
  combined_width int(11),
  combined_height int(11),
  offX int(11),
  offY int(11),
  -- is_stable enum('false','true') NOT NULL DEFAULT 'false',
  period int(11) NOT NULL,
  next_tId int(11) DEFAULT NULL,
--  comment varchar(1000),
  UNIQUE KEY (rle)
--  KEY (next_tId)
--  KEY (comment)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 DATA DIRECTORY = '/home/mysql-gol2' INDEX DIRECTORY = '/home/mysql-gol2';

DROP TABLE IF EXISTS objects;
CREATE TABLE objects (
  oId int(11) NOT NULL AUTO_INCREMENT PRIMARY KEY,
  rle varchar(1000) NOT NULL,
  name varchar(20),
  width int(11) NOT NULL,
  height int(11) NOT NULL,
  dx int(11) NOT NULL,
  dy int(11) NOT NULL,
  dt int(11) NOT NULL,
  UNIQUE KEY (rle)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

