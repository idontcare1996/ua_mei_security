public class Settings {
    private int initialValue;
    private int minimumIncrement;
    private int maximumIncrement;
    private boolean encryptedBidder;
    private boolean encryptedBidVale;
    private int time;
    private boolean encrytedAuthor;

    public Settings() {
    }

    public Settings(int initialValue, int minimumIncrement, int maximumIncrement, boolean encryptedBidder, boolean encryptedBidVale, int time, boolean encrytedAuthor) {
        this.initialValue = initialValue;
        this.minimumIncrement = minimumIncrement;
        this.maximumIncrement = maximumIncrement;
        this.encryptedBidder = encryptedBidder;
        this.encryptedBidVale = encryptedBidVale;
        this.time = time;
        this.encrytedAuthor = encrytedAuthor;
    }

    public int getInitialValue() {
        return initialValue;
    }

    public void setInitialValue(int initialValue) {
        this.initialValue = initialValue;
    }

    public int getMinimumIncrement() {
        return minimumIncrement;
    }

    public void setMinimumIncrement(int minimumIncrement) {
        this.minimumIncrement = minimumIncrement;
    }

    public int getMaximumIncrement() {
        return maximumIncrement;
    }

    public void setMaximumIncrement(int maximumIncrement) {
        this.maximumIncrement = maximumIncrement;
    }

    public boolean isEncryptedBidder() {
        return encryptedBidder;
    }

    public void setEncryptedBidder(boolean encryptedBidder) {
        this.encryptedBidder = encryptedBidder;
    }

    public boolean isEncryptedBidVale() {
        return encryptedBidVale;
    }

    public void setEncryptedBidVale(boolean encryptedBidVale) {
        this.encryptedBidVale = encryptedBidVale;
    }

    public int getTime() {
        return time;
    }

    public void setTime(int time) {
        this.time = time;
    }

    public boolean isEncrytedAuthor() {
        return encrytedAuthor;
    }

    public void setEncrytedAuthor(boolean encrytedAuthor) {
        this.encrytedAuthor = encrytedAuthor;
    }
}
