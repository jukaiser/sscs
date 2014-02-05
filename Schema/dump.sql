-- MySQL dump 10.14  Distrib 5.5.33-MariaDB, for Linux (x86_64)
--
-- Host: localhost    Database: gol
-- ------------------------------------------------------
-- Server version	5.5.33-MariaDB

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `bullets`
--

DROP TABLE IF EXISTS `bullets`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `bullets` (
  `bId` int(11) NOT NULL AUTO_INCREMENT,
  `name` varchar(20) NOT NULL,
  `rle` varchar(30) NOT NULL,
  `width` int(11) NOT NULL,
  `height` int(11) NOT NULL,
  `dx` int(11) NOT NULL,
  `dy` int(11) NOT NULL,
  `dt` int(11) NOT NULL,
  `base_x` int(11) NOT NULL,
  `base_y` int(11) NOT NULL,
  `reference` enum('TOPLEFT','TOPRIGHT','BOTTOMLEFT','BOTTOMRIGHT') DEFAULT NULL,
  `lane_dx` int(11) NOT NULL,
  `lane_dy` int(11) NOT NULL,
  `lanes_per_height` int(11) NOT NULL,
  `lanes_per_width` int(11) NOT NULL,
  `extra_lanes` int(11) NOT NULL,
  PRIMARY KEY (`bId`),
  UNIQUE KEY `name` (`name`),
  KEY `rle` (`rle`)
) ENGINE=MyISAM AUTO_INCREMENT=4 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `bullets`
--

LOCK TABLES `bullets` WRITE;
/*!40000 ALTER TABLE `bullets` DISABLE KEYS */;
INSERT INTO `bullets` VALUES (1,'gl-se','o$b2o$2o!',3,3,1,1,4,-4,0,'BOTTOMLEFT',0,-1,1,1,8),(2,'LWSS-e:0','o2bo$4bo$o3bo$b4o!',5,4,2,0,4,-6,2,'BOTTOMLEFT',0,-1,1,0,7),(3,'LWSS-e:2','b4o$o3bo$4bo$o2bo!',5,4,2,0,4,-6,2,'BOTTOMLEFT',0,-1,1,0,7);
/*!40000 ALTER TABLE `bullets` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `chain`
--

DROP TABLE IF EXISTS `chain`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `chain` (
  `cId` int(11) NOT NULL AUTO_INCREMENT,
  `context` varchar(40) NOT NULL,
  `parent_tId` int(11) DEFAULT NULL,
  `parent_cId` int(11) DEFAULT NULL,
  `rId` int(11) DEFAULT NULL,
  `cost` int(11) NOT NULL,
  PRIMARY KEY (`cId`),
  KEY `context` (`context`),
  KEY `parent_tId` (`parent_tId`),
  KEY `parent_cId` (`parent_cId`),
  KEY `cost` (`cost`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `chain`
--

LOCK TABLES `chain` WRITE;
/*!40000 ALTER TABLE `chain` DISABLE KEYS */;
/*!40000 ALTER TABLE `chain` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `extra_ships`
--

DROP TABLE IF EXISTS `extra_ships`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `extra_ships` (
  `eId` int(11) NOT NULL AUTO_INCREMENT,
  `rId` int(11) NOT NULL,
  `rle` varchar(30) NOT NULL,
  `width` int(11) NOT NULL,
  `height` int(11) NOT NULL,
  `dx` int(11) NOT NULL,
  `dy` int(11) NOT NULL,
  `dt` int(11) NOT NULL,
  PRIMARY KEY (`eId`),
  KEY `rId` (`rId`),
  KEY `rle` (`rle`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `extra_ships`
--

LOCK TABLES `extra_ships` WRITE;
/*!40000 ALTER TABLE `extra_ships` DISABLE KEYS */;
/*!40000 ALTER TABLE `extra_ships` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `reaction`
--

DROP TABLE IF EXISTS `reaction`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `reaction` (
  `rId` int(11) NOT NULL AUTO_INCREMENT,
  `initial_tId` int(11) NOT NULL,
  `bId` int(11) NOT NULL,
  `lane` int(11) NOT NULL,
  `result_tId` int(11) NOT NULL,
  `dx` int(11) NOT NULL,
  `dy` int(11) NOT NULL,
  `dt` int(11) NOT NULL,
  `result` enum('fly-by','stable','unfinished','extraships') DEFAULT NULL,
  PRIMARY KEY (`rId`),
  KEY `result` (`result`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `reaction`
--

LOCK TABLES `reaction` WRITE;
/*!40000 ALTER TABLE `reaction` DISABLE KEYS */;
/*!40000 ALTER TABLE `reaction` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `target`
--

DROP TABLE IF EXISTS `target`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `target` (
  `tId` int(11) NOT NULL AUTO_INCREMENT,
  `rle` varchar(1000) NOT NULL,
  `width` int(11) NOT NULL,
  `height` int(11) NOT NULL,
  `is_stable` enum('false','true') NOT NULL DEFAULT 'false',
  `period` int(11) NOT NULL,
  `next_tId` int(11) DEFAULT NULL,
  PRIMARY KEY (`tId`),
  UNIQUE KEY `rle` (`rle`),
  KEY `next_tId` (`next_tId`)
) ENGINE=MyISAM AUTO_INCREMENT=2 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `target`
--

LOCK TABLES `target` WRITE;
/*!40000 ALTER TABLE `target` DISABLE KEYS */;
INSERT INTO `target` VALUES (1,'!',0,0,'true',1,NULL);
/*!40000 ALTER TABLE `target` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2014-02-05 15:41:33
