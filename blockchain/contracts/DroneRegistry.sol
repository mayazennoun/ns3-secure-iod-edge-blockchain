// SPDX-License-Identifier: MIT
pragma solidity ^0.8.0;

contract DroneRegistry {

    mapping(string => bool) public authorizedDrones;
    mapping(string => string) public dronePublicKeys;

    function registerDrone(string memory droneId, string memory pubKey) public {
        authorizedDrones[droneId] = true;
        dronePublicKeys[droneId] = pubKey;
    }

    function isAuthorized(string memory droneId) public view returns (bool) {
        return authorizedDrones[droneId];
    }

    function getPublicKey(string memory droneId) public view returns (string memory) {
        return dronePublicKeys[droneId];
    }
}