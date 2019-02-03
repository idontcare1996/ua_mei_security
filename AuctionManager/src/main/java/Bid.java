import java.sql.Timestamp;

public class Bid {

    private String auction_number;
    private Long bid_value;
    private String from;
    private Timestamp timestamp;
    private String hash;

    public Bid(String auction_number, Long bid_value, String from)
    {
        this.auction_number = auction_number;
        this.bid_value = bid_value;
        this.from = from;
        this.timestamp = new Timestamp(System.currentTimeMillis()); // TO DO: CHANGE THIS TO THE SAME TIME FOR EVERYONE (timezones and accurate times)
        this.hash = calculateHash();
    }

    private String calculateHash(){
        String output = Hasher.hashSHA256(auction_number+(bid_value)+from+(timestamp));
        return output;
    }

    public String getAuction_number() {
        return auction_number;
    }

    public void setAuction_number(String auction_number) {
        this.auction_number = auction_number;
    }

    public Long getBid_value() {
        return bid_value;
    }

    public void setBid_value(Long bid_value) {
        this.bid_value = bid_value;
    }

    public String getFrom() {
        return from;
    }

    public void setFrom(String from) {
        this.from = from;
    }
}
