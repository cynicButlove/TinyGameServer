CREATE DATABASE game_db;
USE game_db;

CREATE TABLE users (
                       client_id INT PRIMARY KEY AUTO_INCREMENT,
                       username VARCHAR(50) UNIQUE NOT NULL,
                       password VARCHAR(50) NOT NULL
);
