DROP TABLE IF EXISTS bullets;
DROP TABLE IF EXISTS bullet;
CREATE TABLE bullet (
  bId INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(20) NOT NULL,
  oId INT UNSIGNED NOT NULL,		-- object.oId
  base_x TINYINT NOT NULL,
  base_y TINYINT NOT NULL,
  reference ENUM('TOPLEFT','TOPRIGHT','BOTTOMLEFT','BOTTOMRIGHT') DEFAULT NULL,
  lane_dx TINYINT NOT NULL,
  lane_dy TINYINT NOT NULL,
  lanes_per_height TINYINT UNSIGNED NOT NULL,
  lanes_per_width TINYINT UNSIGNED NOT NULL,
  extra_lanes TINYINT UNSIGNED NOT NULL,
  UNIQUE KEY (name)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS chain;
DROP TABLE IF EXISTS transition;
CREATE TABLE transition (
  -- trId INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  rId INT UNSIGNED NOT NULL,			-- reaction.rId
  initial_state TINYINT UNSIGNED NOT NULL,
  result_state TINYINT UNSIGNED NOT NULL,
  rephase TINYINT UNSIGNED NOT NULL,
  pId INT UNSIGNED NOT NULL,			-- part.pId
  cost TINYINT UNSIGNED NOT NULL,
  total_cost TINYINT UNSIGNED NOT NULL,
  PRIMARY KEY (rId, result_state)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 DATA DIRECTORY = '/home/mysql-gol' INDEX DIRECTORY = '/home/mysql-gol';

DROP TABLE IF EXISTS emitted;
CREATE TABLE emitted (
  rId INT UNSIGNED NOT NULL,			-- reaction.rId
  seq TINYINT UNSIGNED NOT NULL,
  oId INT UNSIGNED NOT NULL,
  offX TINYINT NOT NULL,
  offY TINYINT NOT NULL,
  gen SMALLINT UNSIGNED NOT NULL,
  PRIMARY KEY (rId, seq),
  KEY (rId),
  KEY (oId)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 DATA DIRECTORY = '/home/mysql-gol' INDEX DIRECTORY = '/home/mysql-gol';

DROP TABLE IF EXISTS reaction;
CREATE TABLE reaction (
  rId INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  initial_tId INT UNSIGNED NOT NULL,		-- target.tId
  initial_phase TINYINT UNSIGNED NOT NULL,
  bId INT UNSIGNED NOT NULL,			-- bullet.bId
  lane TINYINT UNSIGNED NOT NULL,
  lane_adj TINYINT NOT NULL,
  result_tId INT UNSIGNED NOT NULL,		-- target.tId
  result_phase TINYINT UNSIGNED NOT NULL,
  offX TINYINT NOT NULL,
  offY TINYINT NOT NULL,
  delay TINYINT UNSIGNED NOT NULL,
  gen SMALLINT UNSIGNED NOT NULL,
  min_cost TINYINT UNSIGNED NOT NULL,
  result ENUM('dies','fly-by','stable','unfinished', 'explodes','pruned') DEFAULT NULL,
  emits_ships ENUM('false','true') DEFAULT 'false',
  -- cost TINYINT UNSIGNED NOT NULL,
  -- summary VARCHAR(4000) NOT NULL,
  UNIQUE (initial_tId, initial_phase, bId, lane),
  -- KEY (result),
  -- KEY (initial_tId),
  KEY (result_tId, result_phase)
  -- FULLTEXT INDEX (summary)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 DATA DIRECTORY = '/home/mysql-gol' INDEX DIRECTORY = '/home/mysql-gol';

DROP TABLE IF EXISTS target;
CREATE TABLE target (
  tId INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  hashVal INT UNSIGNED NOT NULL,
  rle VARCHAR(400) NOT NULL,
  period TINYINT UNSIGNED NOT NULL,
  combined_width TINYINT UNSIGNED NOT NULL,	-- just for information - i.e. to make looking for edgeshooters easier
  combined_height TINYINT UNSIGNED NOT NULL,
  KEY (hashVal)
--  comment VARCHAR(1000),
--  KEY (comment)
) ENGINE=MyISAM DEFAULT CHARSET=latin1 DATA DIRECTORY = '/home/mysql-gol' INDEX DIRECTORY = '/home/mysql-gol';

DROP TABLE IF EXISTS objects;
DROP TABLE IF EXISTS object;
CREATE TABLE object (
  oId INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  rle VARCHAR(1000) NOT NULL,
  name VARCHAR(20),
  width TINYINT UNSIGNED NOT NULL,
  height TINYINT UNSIGNED NOT NULL,
  dx TINYINT NOT NULL,
  dy TINYINT NOT NULL,
  dt TINYINT UNSIGNED NOT NULL,
  phase TINYINT UNSIGNED NOT NULL,
  offX TINYINT NOT NULL,
  offY TINYINT NOT NULL,
  UNIQUE KEY (rle)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

DROP TABLE IF EXISTS part;
CREATE TABLE part (
  pId INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  rle VARCHAR(60000),
  ship_name VARCHAR(20),
  part_name VARCHAR(20),
  dx SMALLINT UNSIGNED,
  dy SMALLINT UNSIGNED,
  offX TINYINT,
  offY TINYINT,
  fireX SMALLINT UNSIGNED,
  fireY SMALLINT UNSIGNED,
  type ENUM('track','rephaser','rake') NOT NULL,
  lane_adjust TINYINT UNSIGNED,
  bId INT UNSIGNED,				-- bullet.bId
  lane_fired TINYINT UNSIGNED,
  cost TINYINT UNSIGNED,
  KEY (ship_name)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

