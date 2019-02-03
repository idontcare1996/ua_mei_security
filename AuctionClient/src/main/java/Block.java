import com.google.gson.Gson;

import java.sql.Timestamp;

public class Block {

    public Auction auction;
    public Bid bid;
    public Timestamp timestamp;
    public String hash;
    public String previous_hash;
    public int nonce;

    //Block Constructor.
    public Block(Bid bid, String previous_hash ) {

        //this.auction = auction;
        this.bid = bid;
        this.previous_hash = previous_hash;
        this.timestamp = new Timestamp(System.currentTimeMillis());
        this.hash = calculateHash();
    }
    public Block(Auction auction, String previous_hash)
    {
        this.auction = auction;
        this.previous_hash = previous_hash;
        this.timestamp = new Timestamp(System.currentTimeMillis());
        this.hash = calculateHash();
    }

    //Calculate Hash from given input
    public String calculateHash(){

        Gson auction_gson = new Gson();
        Gson bid_gson = new Gson();

        String auction_string = auction_gson.toJson(auction);
        String bid_string = bid_gson.toJson(bid);

        String output = Hasher.hashSHA256(auction_string + bid_string + previous_hash + (timestamp));
        return output;
    }

}