#include <gtest/gtest.h>
#include "Pointer.h"

using namespace hbthreads;

class TestObject : public Object {
public:
    int value;
    static int instance_count;
    static int destructor_count;

    TestObject(int val) : value(val) {
        instance_count++;
    }

    ~TestObject() override {
        destructor_count++;
    }
};

int TestObject::instance_count = 0;
int TestObject::destructor_count = 0;

class PointerTest : public ::testing::Test {
protected:
    void SetUp() override {
        pool = new boost::container::pmr::monotonic_buffer_resource(64 * 1024ULL);
        buffer = new boost::container::pmr::unsynchronized_pool_resource(pool);
        storage = buffer;
        TestObject::instance_count = 0;
        TestObject::destructor_count = 0;
    }

    void TearDown() override {
        delete buffer;
        delete pool;
        storage = nullptr;
    }

    boost::container::pmr::monotonic_buffer_resource* pool;
    boost::container::pmr::unsynchronized_pool_resource* buffer;
};

TEST_F(PointerTest, DefaultConstructor) {
    Pointer<TestObject> ptr;
    EXPECT_EQ(ptr.get(), nullptr);
}

TEST_F(PointerTest, ConstructorWithObject) {
    Pointer<TestObject> ptr(new TestObject(42));
    EXPECT_NE(ptr.get(), nullptr);
    EXPECT_EQ(ptr->value, 42);
    EXPECT_EQ(TestObject::instance_count, 1);
}

TEST_F(PointerTest, ReferenceCounting) {
    TestObject* obj = new TestObject(42);
    EXPECT_EQ(TestObject::instance_count, 1);
    EXPECT_EQ(TestObject::destructor_count, 0);

    {
        Pointer<TestObject> ptr1(obj);
        EXPECT_EQ(TestObject::destructor_count, 0);

        {
            Pointer<TestObject> ptr2 = ptr1;
            EXPECT_EQ(ptr1.get(), ptr2.get());
            EXPECT_EQ(TestObject::destructor_count, 0);
        }
        // ptr2 destroyed, but object still alive
        EXPECT_EQ(TestObject::destructor_count, 0);
    }
    // ptr1 destroyed, object should be deleted
    EXPECT_EQ(TestObject::destructor_count, 1);
}

TEST_F(PointerTest, MultipleReferences) {
    Pointer<TestObject> ptr1(new TestObject(100));
    Pointer<TestObject> ptr2 = ptr1;
    Pointer<TestObject> ptr3 = ptr2;

    EXPECT_EQ(ptr1.get(), ptr2.get());
    EXPECT_EQ(ptr2.get(), ptr3.get());
    EXPECT_EQ(ptr1->value, 100);
    EXPECT_EQ(ptr2->value, 100);
    EXPECT_EQ(ptr3->value, 100);

    EXPECT_EQ(TestObject::destructor_count, 0);
}

TEST_F(PointerTest, Assignment) {
    Pointer<TestObject> ptr1(new TestObject(1));
    Pointer<TestObject> ptr2(new TestObject(2));

    EXPECT_EQ(ptr1->value, 1);
    EXPECT_EQ(ptr2->value, 2);
    EXPECT_EQ(TestObject::instance_count, 2);

    ptr1 = ptr2;
    EXPECT_EQ(ptr1->value, 2);
    EXPECT_EQ(ptr2->value, 2);
    EXPECT_EQ(ptr1.get(), ptr2.get());

    // First object should be deleted
    EXPECT_EQ(TestObject::destructor_count, 1);
}

TEST_F(PointerTest, OperatorEqual) {
    Pointer<TestObject> ptr1(new TestObject(42));
    Pointer<TestObject> ptr2 = ptr1;
    Pointer<TestObject> ptr3(new TestObject(43));
    Pointer<TestObject> ptr4;

    EXPECT_TRUE(ptr1 == ptr2);
    EXPECT_FALSE(ptr1 == ptr3);
    EXPECT_FALSE(ptr1 == ptr4);
}

TEST_F(PointerTest, NullPointer) {
    Pointer<TestObject> ptr;
    EXPECT_EQ(ptr.get(), nullptr);

    ptr = Pointer<TestObject>(new TestObject(42));
    EXPECT_NE(ptr.get(), nullptr);

    ptr = Pointer<TestObject>();
    EXPECT_EQ(ptr.get(), nullptr);
}

TEST_F(PointerTest, SelfAssignment) {
    Pointer<TestObject> ptr(new TestObject(42));
    ptr = ptr;  // Self-assignment
    EXPECT_EQ(ptr->value, 42);
    EXPECT_EQ(TestObject::destructor_count, 0);
}

TEST_F(PointerTest, AllocationDeallocation) {
    const int COUNT = 100;
    for (int i = 0; i < COUNT; i++) {
        Pointer<TestObject> ptr(new TestObject(i));
        EXPECT_EQ(ptr->value, i);
    }
    EXPECT_EQ(TestObject::instance_count, COUNT);
    EXPECT_EQ(TestObject::destructor_count, COUNT);
}

TEST_F(PointerTest, NestedAllocation) {
    class Container : public Object {
    public:
        Pointer<TestObject> child;
        Container() {
            child = Pointer<TestObject>(new TestObject(999));
        }
    };

    {
        Pointer<Container> container(new Container);
        EXPECT_EQ(container->child->value, 999);
    }
    // Both container and child should be deleted
    EXPECT_EQ(TestObject::destructor_count, 1);
}

// Death test for null storage check (our recent fix)
TEST(PointerDeathTest, NullStorageCheck) {
    storage = nullptr;
    ASSERT_DEATH({
        TestObject* obj = new TestObject(42);
        (void)obj;
    }, "storage must be initialized");
}

TEST_F(PointerTest, HashSupport) {
    Pointer<TestObject> ptr1(new TestObject(42));
    Pointer<TestObject> ptr2 = ptr1;
    Pointer<TestObject> ptr3(new TestObject(43));

    std::hash<Pointer<TestObject>> hasher;
    EXPECT_EQ(hasher(ptr1), hasher(ptr2));
    EXPECT_NE(hasher(ptr1), hasher(ptr3));
}