public class Settings {
    private double initialValue;
    private double minimumIncrement;
    private double maximumIncrement;
    private boolean encryptedBidder;
    private boolean encryptedBidValue;
    private int time;
    private boolean encryptedAuthor;

    public Settings() {
    }

    public Settings(double initialValue, double minimumIncrement, double maximumIncrement, boolean encryptedBidder, boolean encryptedBidValue, int time, boolean encrytedAuthor) {
        this.initialValue = initialValue;
        this.minimumIncrement = minimumIncrement;
        this.maximumIncrement = maximumIncrement;
        this.encryptedBidder = encryptedBidder;
        this.encryptedBidValue = encryptedBidValue;
        this.time = time;
        this.encryptedAuthor = encrytedAuthor;
    }

    public double getInitialValue() {
        return initialValue;
    }

    public void setInitialValue(double initialValue) {
        this.initialValue = initialValue;
    }

    public double getMinimumIncrement() {
        return minimumIncrement;
    }

    public void setMinimumIncrement(double minimumIncrement) {
        this.minimumIncrement = minimumIncrement;
    }

    public double getMaximumIncrement() {
        return maximumIncrement;
    }

    public void setMaximumIncrement(double maximumIncrement) {
        this.maximumIncrement = maximumIncrement;
    }

    public boolean isEncryptedBidder() {
        return encryptedBidder;
    }

    public void setEncryptedBidder(boolean encryptedBidder) {
        this.encryptedBidder = encryptedBidder;
    }

    public boolean isEncryptedBidValue() {
        return encryptedBidValue;
    }

    public void setEncryptedBidValue(boolean encryptedBidValue) {
        this.encryptedBidValue = encryptedBidValue;
    }

    public int getTime() {
        return time;
    }

    public void setTime(int time) {
        this.time = time;
    }

    public boolean isEncryptedAuthor() {
        return encryptedAuthor;
    }

    public void setEncryptedAuthor(boolean encryptedAuthor) {
        this.encryptedAuthor = encryptedAuthor;
    }
}
