import java.sql.Timestamp;

public class Auction {

    private String id;
    private String seller;
    private String product;
    private String description;
    private String settings;
    private Timestamp timestamp;
    private String hash;

    public Auction(){

    }

    public Auction(String seller, String product, String description, String settings, Timestamp timestamp) {
        this.seller = seller;
        this.product = product;
        this.description = description;
        this.settings = settings;
        this.timestamp = timestamp;
    }

    public Auction(String id, String seller, String product, String description, String settings)
    {
        this.id = id;
        this.seller = seller;
        this.product = product;
        this.description = description;
        this.settings = settings;
        this.timestamp = new Timestamp(System.currentTimeMillis()); // TO DO: CHANGE THIS TO THE SAME TIME FOR EVERYONE (timezones and accurate times)
        this.hash = calculateHash();
    }
    public String calculateHash(){
        String output = Hasher.hashSHA256(id+seller+product+description+settings+
                (timestamp));
        return output;
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


