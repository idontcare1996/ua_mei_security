import java.sql.SQLOutput;
import java.util.ArrayList; //To use the Linked List that is the base of a Blockchain
import com.google.gson.GsonBuilder; //To write/parse JSON





public class Repository {

    //Create ArrayList, named blockchain with objects of type Block
    public static ArrayList<Block> blockchain = new ArrayList<Block>();

    //Ammount of blocks to add to the blockchain, for testing
    private static final int BLOCKSTOADD = 4;

    public static void main(String[] args) {

        //Adding the genesis block to the blockchain
        blockchain.add(new Block("Data1","0")); // First block's previous hash is 0 because it's the genesis block.

        //Adding remaining blocks to the blockchain, and setting the "previous_hash" to the previous item in the blockchain's hash.
        for(int i=1; i < BLOCKSTOADD; i++) {
            blockchain.add(new Block("Data"+i, blockchain.get(blockchain.size() - 1).hash));
        }

        //Adds an element in the blockchain to invalidade it. It gets between position 1 and 2.
        //blockchain.add(2, new Block("Data0",blockchain.get(1).hash));


        //Stores the blockchain in JSON format, with PrettyPrinting on(facilitating human reading).
        String blockchainJSON = new GsonBuilder().setPrettyPrinting().create().toJson(blockchain);

        //Prints the JSON
        System.out.println(blockchainJSON);

        System.out.println("Is the blockchain valid: "+isBlockChainValid());

    }

    //Method to test blockchain's validity
    public static Boolean isBlockChainValid()
    {
        //Create the Blocks objects to use next
        Block previous_block,current_block;

        //Check each element of the blockchain for it's hash (starts at 1, because the first element has "0" as previous hash - the genesis block
        for(int i=1; i < blockchain.size(); i++){

            current_block = blockchain.get(i);
            previous_block = blockchain.get(i-1);

            //compare the hash on the block to the calculated hash:
            if(!current_block.hash.equals(current_block.calculateHash())){
                System.out.println("Current hash is different for block in position number "+i+" in the blockchain.");
                return false;
            }
            if(!previous_block.hash.equals(current_block.previous_hash)){
                System.out.println("Previous hash is different for block in position number "+i+" in the blockchain.");
                return false;
            }

        }
        return true;
    }

}
