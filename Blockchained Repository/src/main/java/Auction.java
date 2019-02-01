import java.sql.Timestamp;

public class Auction {

    private String id;
    private String type;
    private String seller;
    private String product;
    private String settings;
    private Timestamp timestamp;
    private String hash;

    public Auction(String id, String type, String seller,String product, String settings)
    {
        this.id = id;
        this.type = type;
        this.seller = seller;
        this.product = product;
        this.settings = settings;
        this.timestamp = new Timestamp(System.currentTimeMillis()); // TO DO: CHANGE THIS TO THE SAME TIME FOR EVERYONE (timezones and accurate times)
        this.hash = calculateHash();
    }
    public String calculateHash(){
        String output = Hasher.hashSHA256(id+type+seller+product+settings+
                (timestamp));
        return output;
    }
    public String getType() {
        return type;
    }

    public void setType(String type) {
        this.type = type;
    }

    public String getSeller() {
        return seller;
    }

    public void setSeller(String seller) {
        this.seller = seller;
    }

    public String getId() {
        return id;
    }

    public void setId(String id) {
        this.id = id;
    }

    public String getProduct() {
        return product;
    }

    public void setProduct(String product) {
        this.product = product;
    }

    public String getSettings() {
        return settings;
    }

    public void setSettings(String data) {
        this.settings = settings;
    }

    public Timestamp getTimestamp() {
        return timestamp;
    }

    public void setTimestamp(Timestamp timestamp) {
        this.timestamp = timestamp;
    }
}


