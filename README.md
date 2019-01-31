# Auction System

## Auction system consisting of Client, Manager and Repository

+ ### Client

The Client is the application through which the user will engage in all the auction actions. This will allow the user to create/participate in auctions. The application will require, in a near future, the use of a **Portuguese Citizenship Card (CC)** - a Smart Card with the user's credentials in order to access the application's interface and functionality.

+ ### Manager

The Manager is responsible for creating auctions in the Repository upon a client's requests. It's an **HTTP Server**, like the Repository, that also performs additional functions, such as performing additional validations on bids if requested by the Repository as well as receiving dynamic code as input for the creation and validation processes needed for a correct auction.

+ ### Repository

Currently, the repository module is under development in the BlockchainedRepository module, where a blockchain was implemented.
This includes a Hashing class, which uses **SHA256** to hash a given string, the Block class, with its constructor, and method to hash its contents. There is also a method in the Main class of this module, which validates the blockchain.
The "normal" Repository currently has a simple **HTTP server** running, in order to receive JSON from the Client application. This is done through POSTs from the later.
The "normal" Repository currently has a simple **HTTP server** running, in order to receive JSON from the Client application. This is done through POSTs from the later.

+ ### Additional information

Further instructions can be obtained by reading the comments inside each source file, since documentation for this project is not a priority.