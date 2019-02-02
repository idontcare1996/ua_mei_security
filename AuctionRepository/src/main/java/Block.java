import java.sql.Timestamp;

public class Block {

    public String hash;
    public String previous_hash;
    private String data;
    private Timestamp timestamp;
    private int nonce;

    //Block Constructor.
    public Block(String data,String previous_hash ) {
        this.data = data;
        this.previous_hash = previous_hash;
        this.timestamp = new Timestamp(System.currentTimeMillis());
        this.hash = calculateHash();
    }

    //Calculate Hash from given input
    public String calculateHash(){
        String output = Hasher.hashSHA256(previous_hash +
                                                (timestamp)+
                                                data);
        return output;
    }

}