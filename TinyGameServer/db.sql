CREATE DATABASE game_db;
USE game_db;

CREATE TABLE users (
                       client_id INT PRIMARY KEY AUTO_INCREMENT,
                       username VARCHAR(50) UNIQUE NOT NULL,
                       password VARCHAR(50) NOT NULL
);
CREATE TABLE ranklist (
                         client_id INT PRIMARY KEY,
                         score INT NOT NULL,
                         FOREIGN KEY (client_id) REFERENCES users(client_id)
);