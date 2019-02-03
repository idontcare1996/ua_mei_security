import java.sql.Timestamp;
import java.util.UUID;

public class Bid {

    private String bid_id;
    private String auction_number;
    private String bid_value;
    private String from;
    private Timestamp timestamp;
    private String hash;

    public Bid (String auction_number, String bid_value, String from)
    {
        UUID bid_uuid = UUID.randomUUID();
        this.bid_id=bid_uuid.toString();
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

    public String getBid_id() {
        return bid_id;
    }

    public void setBid_id(String bid_id) {
        this.bid_id = bid_id;
    }

    public String getAuction_number() {
        return auction_number;
    }

    public void setAuction_number(String auction_number) {
        this.auction_number = auction_number;
    }

    public String getBid_value() {
        return bid_value;
    }

    public void setBid_value(String bid_value) {
        this.bid_value = bid_value;
    }

    public String getFrom() {
        return from;
    }

    public void setFrom(String from) {
        this.from = from;
    }
}
